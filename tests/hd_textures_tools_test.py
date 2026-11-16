import argparse
import importlib.machinery
import importlib.util
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
from PIL import Image

root = Path(__file__).resolve().parent.parent
loader = importlib.machinery.SourceFileLoader("hd_textures", str(root / "tools/hd-textures"))
spec = importlib.util.spec_from_loader("hd_textures", loader)
tool = importlib.util.module_from_spec(spec)
loader.exec_module(tool)
KEY = (0, 255, 0)


def stock_with_key_block() -> np.ndarray:
    stock = np.full((256, 256, 3), 200, np.uint8)
    stock[64:128, 64:128] = KEY
    return stock


def pack_alpha_follows_the_stock_key_mask() -> None:
    image = np.full((1024, 1024, 4), 90, np.uint8)
    page = tool.pack_page(image, stock_with_key_block())
    expected = np.full((1024, 1024), 255, np.uint8)
    expected[256:512, 256:512] = 0
    assert (page[..., 3] == expected).all()


def pack_overrides_alpha_the_upscaler_produced() -> None:
    image = np.zeros((1024, 1024, 4), np.uint8)
    image[..., 3] = 128
    page = tool.pack_page(image, stock_with_key_block())
    assert set(np.unique(page[..., 3])) == {0, 255}


def pack_fills_transparent_texels_from_their_opaque_neighbours() -> None:
    image = np.zeros((512, 512, 4), np.uint8)
    image[..., :3] = KEY
    image[:, :128, :3] = (220, 30, 30)
    stock = np.zeros((256, 256, 3), np.uint8)
    stock[...] = KEY
    stock[:, :64] = (220, 30, 30)
    page = tool.pack_page(image, stock)
    transparent = page[..., 3] == 0
    assert transparent[:, 128:].all()
    assert not tool.is_key(page[transparent][..., :3]).any()
    assert (page[:, 128, :3] == (220, 30, 30)).all()


def pack_fills_transparent_texels_from_nearby_opaque_colours() -> None:
    stock = np.zeros((256, 256, 3), np.uint8)
    stock[...] = KEY
    stock[:, :32] = (220, 30, 30)
    stock[:, 224:] = (30, 30, 220)
    image = np.dstack([stock.repeat(2, 0).repeat(2, 1), np.full((512, 512), 255, np.uint8)])
    page = tool.pack_page(image, stock)
    assert (page[:, 70, :3] == (220, 30, 30)).all()
    assert (page[:, 440, :3] == (30, 30, 220)).all()


def key_matches_the_engines_5_bit_test() -> None:
    texels = np.array([[(7, 248, 7), (0, 255, 0), (8, 248, 0), (0, 247, 0), (0, 248, 8)]], np.uint8)
    assert tool.is_key(texels).tolist() == [[True, True, False, False, False]]


def pack_writes_the_engines_names_into_its_folder_and_fails_on_unknown_pages() -> None:
    with tempfile.TemporaryDirectory() as scratch:
        scratch = Path(scratch)
        data, source = scratch / "data", scratch / "png"
        output = data / "HD/Upscaled"
        (data / "ISLAND").mkdir(parents=True)
        (data / "ISLAND/ISLAND00.RAW").write_bytes(bytes(256 * 256 * 3))
        (data / "ISLAND/ISLAND.PLY").write_bytes(bytes(320 * 384 * 3))
        (source / "ISLAND").mkdir(parents=True)
        page = Image.new("RGBA", (512, 512), (90, 90, 90, 255))
        for name in ["ISLAND00", "ISLAND.PLY.0", "ISLAND09"]:
            page.save(source / "ISLAND" / f"{name}.png")
        result = subprocess.run([str(root / "tools/hd-textures"), "--data", str(data), "pack", str(source), "Upscaled"],
                                capture_output=True, text=True)
        assert result.returncode == 1
        assert "ISLAND/ISLAND09: no stock page with this name" in result.stdout
        written = sorted(path.relative_to(output).as_posix() for path in output.rglob("*.RGBA"))
        assert written == ["ISLAND/ISLAND.PLY.0.RGBA", "ISLAND/ISLAND00.RGBA"]
        assert (output / "ISLAND/ISLAND00.RGBA").stat().st_size == 512 * 512 * 4


def resize_writes_the_chosen_pages_scaled_and_inverted() -> None:
    with tempfile.TemporaryDirectory() as scratch:
        scratch = Path(scratch)
        data, output = scratch / "data", scratch / "png"
        (data / "ISLAND").mkdir(parents=True)
        (data / "CITY").mkdir()
        (data / "ISLAND/ISLAND00.RAW").write_bytes(bytes([10, 20, 30]) * (256 * 256))
        (data / "CITY/CITY00.RAW").write_bytes(bytes(256 * 256 * 3))
        result = subprocess.run([str(root / "tools/hd-textures"), "--data", str(data), "resize", "--scale", "2", "--invert",
                                 "--only", "ISLAND/", "--out", str(output)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        assert sorted(path.relative_to(output).as_posix() for path in output.rglob("*.png")) == ["ISLAND/ISLAND00.png"]
        image = np.asarray(Image.open(output / "ISLAND/ISLAND00.png"))
        assert image.shape == (512, 512, 4) and tuple(image[0, 0]) == (245, 235, 225, 255)


def dump_writes_every_stock_page_with_the_key_transparent() -> None:
    with tempfile.TemporaryDirectory() as scratch:
        scratch = Path(scratch)
        data, output = scratch / "data", scratch / "png"
        (data / "ISLAND").mkdir(parents=True)
        (data / "HD/Upscaled/ISLAND").mkdir(parents=True)
        (data / "ISLAND/ISLAND00.RAW").write_bytes(bytes(KEY) + bytes([10, 20, 30]) * (256 * 256 - 1))
        (data / "HD/Upscaled/ISLAND/ISLAND00.RAW").write_bytes(bytes(256 * 256 * 3))
        result = subprocess.run([str(root / "tools/hd-textures"), "--data", str(data), "dump", "--out", str(output)],
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        assert sorted(path.relative_to(output).as_posix() for path in output.rglob("*.png")) == ["ISLAND/ISLAND00.png"]
        image = np.asarray(Image.open(output / "ISLAND/ISLAND00.png"))
        assert image[0, 0, 3] == 0 and tuple(image[0, 1]) == (10, 20, 30, 255)


def pack_names_follow_the_engines_rule() -> None:
    for name in ["Upscaled", "4x UltraSharp", "x" * 63]:
        assert tool.pack_name(name) == name
    for name in ["", ".", "..", ".hidden", "a/b", "a\\b", "a\tb", "x\x7f", "x" * 64]:
        try:
            tool.pack_name(name)
        except argparse.ArgumentTypeError:
            continue
        raise AssertionError(f"accepted {name!r}")


def pack_refuses_a_name_outside_hd() -> None:
    with tempfile.TemporaryDirectory() as scratch:
        data = Path(scratch) / "data"
        (data / "ISLAND").mkdir(parents=True)
        result = subprocess.run([str(root / "tools/hd-textures"), "--data", str(data), "pack", scratch, ".."],
                                capture_output=True, text=True)
        assert result.returncode == 2 and "cannot name a pack" in result.stderr
        assert not (data / "HD").exists()


def pack_rejects_pages_the_engine_would_skip() -> None:
    for shape in [(1000, 1000, 4), (1024, 512, 4), (256, 256, 4), (4096, 4096, 4)]:
        try:
            tool.pack_page(np.zeros(shape, np.uint8), stock_with_key_block())
        except ValueError:
            continue
        raise AssertionError(f"accepted {shape}")


def playfield_tiles_land_where_the_engine_puts_them() -> None:
    sheet = np.zeros((12 * 32, 10 * 32, 3), np.uint8)
    for index in range(120):
        row, column = divmod(index, 10)
        sheet[row * 32:(row + 1) * 32, column * 32:(column + 1) * 32] = (index, 0, 0)
    pages = tool.playfield_pages(sheet.tobytes())
    assert pages[0][0, 0, 0] == 0
    assert pages[0][32, 32, 0] == 9
    assert pages[0][7 * 32, 7 * 32, 0] == 63
    assert pages[1][0, 0, 0] == 64
    assert pages[1][6 * 32, 7 * 32, 0] == 119


def resort_island_resize_pack_matches_the_stock_masks() -> None:
    stock = {name: page for name, page in tool.stock_pages(root / "gamedata")
             if name.startswith(("ISLAND/", "GENERAL/PLAYER"))}
    assert len(stock) == 19, sorted(stock)
    for name, page in stock.items():
        image = tool.rgba(page).repeat(4, 0).repeat(4, 1)
        packed = tool.pack_page(image, page)
        mask = (~tool.is_key(page)).repeat(4, 0).repeat(4, 1)
        assert ((packed[..., 3] == 255) == mask).all(), name
        assert not tool.is_key(packed[..., :3][packed[..., 3] == 0]).any(), name


tests = [
    pack_alpha_follows_the_stock_key_mask,
    pack_overrides_alpha_the_upscaler_produced,
    pack_fills_transparent_texels_from_their_opaque_neighbours,
    pack_fills_transparent_texels_from_nearby_opaque_colours,
    key_matches_the_engines_5_bit_test,
    pack_writes_the_engines_names_into_its_folder_and_fails_on_unknown_pages,
    pack_names_follow_the_engines_rule,
    pack_refuses_a_name_outside_hd,
    resize_writes_the_chosen_pages_scaled_and_inverted,
    dump_writes_every_stock_page_with_the_key_transparent,
    pack_rejects_pages_the_engine_would_skip,
    playfield_tiles_land_where_the_engine_puts_them,
    resort_island_resize_pack_matches_the_stock_masks,
]
for test in tests:
    test()
print(f"{len(tests)} HD texture tool tests passed")
sys.exit(0)
