import importlib.machinery
import importlib.util
import sys
from pathlib import Path

import numpy as np

root = Path(__file__).resolve().parent.parent
loader = importlib.machinery.SourceFileLoader("hd_upscale", str(root / "tools/hd-upscale"))
spec = importlib.util.spec_from_loader("hd_upscale", loader)
upscale = importlib.util.module_from_spec(spec)
loader.exec_module(upscale)


def nearest(image: np.ndarray) -> np.ndarray:
    return image.repeat(4, 0).repeat(4, 1)


def blurred(image: np.ndarray) -> np.ndarray:
    """A stand-in model whose output at an edge depends on the padding beyond it."""
    big = nearest(image).astype(np.float64)
    for axis in (0, 1):
        big = sum(np.roll(big, shift, axis) for shift in range(-6, 7)) / 13
    return np.clip(np.rint(big), 0, 255).astype(np.uint8)


def noise(shape: tuple[int, ...], seed: int) -> np.ndarray:
    return np.random.default_rng(seed).integers(0, 256, shape, dtype=np.uint8)


def split_then_merge_returns_each_kind_of_page_unchanged() -> None:
    page = np.dstack([noise((256, 256, 3), 1), np.full((256, 256), 255, np.uint8)])
    for name in ["ISLAND/ISLAND00", "ISLAND/ISLAND.PLY.0", "GENERAL/PLAYER00"]:
        assert (upscale.upscale_page(page, name, nearest) == nearest(page[..., :3])).all(), name


def axis_sources_read_on_wrap_or_repeat_the_edge() -> None:
    sources = upscale.axis_sources(64, 64, "neighbour", "wrap")
    assert sources[:16].tolist() == list(range(48, 64))
    assert sources[16:80].tolist() == list(range(64, 128))
    assert sources[80:].tolist() == list(range(64, 80))
    sources = upscale.axis_sources(64, 64, "edge", "edge")
    assert sources[:16].tolist() == [64] * 16 and sources[80:].tolist() == [127] * 16


def waves(period: int, phase: float) -> np.ndarray:
    """A smooth pattern that repeats every `period` texels in both directions, like a tiling texture."""
    y, x = np.mgrid[0:period, 0:period] * (2 * np.pi / period)
    wave = 128 + 60 * np.sin(x + phase) + 60 * np.cos(y + 2 * phase)
    return np.repeat(np.rint(wave).astype(np.uint8)[..., None], 3, axis=2)


def padding_follows_the_page_across_a_seam_wraps_a_tile_and_otherwise_repeats_the_edge() -> None:
    page = np.full((256, 256, 3), 255, np.uint8)
    # A ramp across two cells, a cell that tiles with itself, and a ramp that ends beside a flat cell.
    page[0:64, 0:128] = np.linspace(0, 255, 128).astype(np.uint8)[None, :, None]
    page[64:128, 64:128] = waves(64, 0.5)
    page[128:192, 128:192] = np.linspace(0, 200, 64).astype(np.uint8)[None, :, None]
    assert upscale.cell_padding(page, 0, 0, 64)["right"] == "neighbour"
    assert upscale.cell_padding(page, 0, 64, 64)["left"] == "neighbour"
    tiled = upscale.cell_padding(page, 64, 64, 64)
    assert tiled["left"] == tiled["right"] == tiled["top"] == tiled["bottom"] == "wrap", tiled
    assert upscale.cell_padding(page, 128, 128, 64)["right"] == "edge"


def a_cell_that_tiles_still_tiles_after_upscaling() -> None:
    tile = waves(64, 1.0)
    page = np.dstack([np.full((256, 256, 3), 255, np.uint8), np.full((256, 256), 255, np.uint8)])
    page[64:128, 64:128, :3] = tile
    cell = upscale.upscale_page(page, "ISLAND/ISLAND00", blurred)[256:512, 256:512].astype(np.int16)
    # The same cell upscaled in the middle of copies of itself, which is how it sits on a tiled surface.
    surface = blurred(np.tile(tile, (3, 3, 1)))[256:512, 256:512].astype(np.int16)
    assert np.abs(cell - surface).max() <= 2


def ground_tiles_are_mirrored_and_ignore_their_neighbours() -> None:
    page = np.dstack([noise((256, 256, 3), 6), np.full((256, 256), 255, np.uint8)])
    tile = page[32:64, 32:64, :3]
    mirrored = blurred(np.pad(tile, ((16, 16), (16, 16), (0, 0)), mode="symmetric"))[64:192, 64:192]
    result = upscale.upscale_page(page, "ISLAND/ISLAND.PLY.0", blurred)
    assert (result[128:256, 128:256] == mirrored).all()
    page[32:64, 64:96, :3] = 0
    assert (upscale.upscale_page(page, "ISLAND/ISLAND.PLY.0", blurred)[128:256, 128:256] == mirrored).all()


def character_pages_go_whole_and_reflect_at_the_border() -> None:
    page = np.dstack([noise((256, 256, 3), 7), np.full((256, 256), 255, np.uint8)])
    reflected = blurred(np.pad(page[..., :3], ((16, 16), (16, 16), (0, 0)), mode="reflect"))[64:1088, 64:1088]
    assert (upscale.upscale_page(page, "GENERAL/PLAYER00", blurred) == reflected).all()


def a_neighbour_loses_to_a_smoother_wrap() -> None:
    page = np.full((256, 256, 3), 255, np.uint8)
    page[0:64, 0:64] = 100
    page[0:64, 64:128] = 103
    assert upscale.cell_padding(page, 0, 0, 64)["right"] == "wrap"


def a_flat_edge_reads_on_across_a_small_step_but_not_a_large_one() -> None:
    page = np.full((256, 256, 3), 255, np.uint8)
    # Flat at the right edge, so the steps inside are zero and only SEAM_LEVELS decides; far from wrapping.
    page[0:64, 0:64] = 0
    page[0:64, 32:64] = 100
    page[0:64, 64:128] = 103
    page[64:128, 0:64] = 0
    page[64:128, 32:64] = 100
    page[64:128, 64:128] = 110
    assert upscale.cell_padding(page, 0, 0, 64)["right"] == "neighbour"
    assert upscale.cell_padding(page, 64, 0, 64)["right"] == "edge"


tests = [
    split_then_merge_returns_each_kind_of_page_unchanged,
    axis_sources_read_on_wrap_or_repeat_the_edge,
    padding_follows_the_page_across_a_seam_wraps_a_tile_and_otherwise_repeats_the_edge,
    a_cell_that_tiles_still_tiles_after_upscaling,
    ground_tiles_are_mirrored_and_ignore_their_neighbours,
    character_pages_go_whole_and_reflect_at_the_border,
    a_neighbour_loses_to_a_smoother_wrap,
    a_flat_edge_reads_on_across_a_small_step_but_not_a_large_one,
]
for test in tests:
    test()
print(f"{len(tests)} HD upscale tests passed")
sys.exit(0)
