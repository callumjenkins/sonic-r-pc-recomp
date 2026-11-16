package org.sonicr.recomp;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Arrays;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class GameDataImporterTest {
    private static final String[] FOLDERS = {
        "AI", "BIN", "CITY", "EMERALD", "FACTORY", "GENERAL", "ISLAND", "RUIN", "SOUND"
    };

    private static Map<String, byte[]> valid() {
        Map<String, byte[]> files = new LinkedHashMap<>();
        for (String folder : FOLDERS) files.put(folder.toLowerCase() + "/test.bin", new byte[]{1});
        files.put("Music/TRACK2.ADX", new byte[]{2, 3});
        return files;
    }

    private static byte[] zip(Map<String, byte[]> files) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
            for (Map.Entry<String, byte[]> file : files.entrySet()) {
                zip.putNextEntry(new ZipEntry(file.getKey()));
                zip.write(file.getValue());
                zip.closeEntry();
            }
        }
        return bytes.toByteArray();
    }

    private static void rejected(Map<String, byte[]> files, File directory) throws IOException {
        try {
            GameDataImporter.importZip(new ByteArrayInputStream(zip(files)), directory);
        } catch (IOException expected) {
            return;
        }
        throw new AssertionError("Invalid import was accepted");
    }

    public static void main(String[] args) throws Exception {
        Path root = Files.createTempDirectory("sonicr-import-test");
        File directory = root.resolve("gamedata").toFile();
        try {
            assert !GameDataImporter.isReady(directory);
            GameDataImporter.importZip(new ByteArrayInputStream(zip(valid())), directory);
            assert GameDataImporter.isReady(directory);
            assert new File(directory, "BIN/TEST.BIN").isFile();
            assert new File(directory, "MUSIC/track2.adx").isFile();

            Map<String, byte[]> invalid = valid();
            invalid.put("BIN/../../escape", new byte[]{9});
            rejected(invalid, directory);
            assert !root.resolve("escape").toFile().exists();
            assert GameDataImporter.isReady(directory);
            invalid = valid();
            invalid.put("/absolute", new byte[]{9});
            rejected(invalid, directory);
            invalid = valid();
            invalid.put("BIN\\..\\..\\escape", new byte[]{9});
            rejected(invalid, directory);
            invalid = valid();
            invalid.put("BIN/TEST.BIN", new byte[]{9});
            rejected(invalid, directory);
            invalid = valid();
            invalid.put("BIN//TEST.BIN", new byte[]{9});
            rejected(invalid, directory);
            invalid = valid();
            invalid.put("BIN/../BIN/OTHER.BIN", new byte[]{9});
            rejected(invalid, directory);
            invalid = valid();
            invalid.put("BIN/./OTHER.BIN", new byte[]{9});
            rejected(invalid, directory);
            for (String folder : FOLDERS) {
                invalid = valid();
                invalid.remove(folder.toLowerCase() + "/test.bin");
                rejected(invalid, directory);
            }
            assert Files.readAllBytes(root.resolve("gamedata/BIN/TEST.BIN"))[0] == 1;

            Path save = root.resolve("gamedata/SAVE/SONICR.SAV");
            Files.createDirectories(save.getParent());
            Files.write(save, new byte[]{4, 5, 6});
            Files.write(root.resolve("gamedata/SONICR.INF"), new byte[]{7});
            Map<String, byte[]> updated = valid();
            updated.put("bin/test.bin", new byte[]{8});
            GameDataImporter.importZip(new ByteArrayInputStream(zip(updated)), directory);
            assert Arrays.equals(Files.readAllBytes(save), new byte[]{4, 5, 6});
            assert Files.readAllBytes(root.resolve("gamedata/SONICR.INF"))[0] == 7;
            assert Files.readAllBytes(root.resolve("gamedata/BIN/TEST.BIN"))[0] == 8;
            assert !root.resolve("gamedata-previous").toFile().exists();
            assert !root.resolve("gamedata-import").toFile().exists();

            Map<String, byte[]> withPack = valid();
            withPack.put("hd/Upscaled/island/island00.rgba", new byte[]{1});
            GameDataImporter.importZip(new ByteArrayInputStream(zip(withPack)), directory);
            assert new File(directory, "HD/Upscaled/ISLAND/ISLAND00.RGBA").isFile();
            withPack = valid();
            withPack.put("HD/Inverted/ISLAND/ISLAND01.RGBA", new byte[]{2});
            GameDataImporter.importZip(new ByteArrayInputStream(zip(withPack)), directory);
            assert new File(directory, "HD/Inverted/ISLAND/ISLAND01.RGBA").isFile();
            assert !new File(directory, "HD/Upscaled").exists();
            Map<String, byte[]> packOnly = new LinkedHashMap<>();
            packOnly.put("HD/Upscaled/ISLAND/ISLAND00.RGBA", new byte[]{1});
            rejected(packOnly, directory);
            assert new File(directory, "HD/Inverted/ISLAND/ISLAND01.RGBA").isFile();
            assert Arrays.equals(Files.readAllBytes(save), new byte[]{4, 5, 6});

            assert directory.renameTo(root.resolve("gamedata-previous").toFile());
            GameDataImporter.importZip(new ByteArrayInputStream(zip(updated)), directory);
            assert Arrays.equals(Files.readAllBytes(save), new byte[]{4, 5, 6});
            assert directory.renameTo(root.resolve("gamedata-previous").toFile());
            GameDataImporter.recover(directory);
            assert GameDataImporter.isReady(directory);
            assert Arrays.equals(Files.readAllBytes(save), new byte[]{4, 5, 6});

            long limit = 1024L * 1024 * 1024;
            assert GameDataImporter.checkedTotal(limit - 1, 1) == limit;
            try {
                GameDataImporter.checkedTotal(limit, 1);
                throw new AssertionError("Oversized game data was accepted");
            } catch (IOException expected) {
                assert expected.getMessage().contains("1 GB");
            }

            Map<String, byte[]> many = valid();
            for (int i = 0; i < 9990; i++) many.put("unused/" + i, new byte[0]);
            GameDataImporter.importZip(new ByteArrayInputStream(zip(many)), directory);
            many.put("unused/extra", new byte[0]);
            rejected(many, directory);
            assert Arrays.equals(Files.readAllBytes(save), new byte[]{4, 5, 6});

            byte[] previousAsset = Files.readAllBytes(root.resolve("gamedata/BIN/TEST.BIN"));
            byte[] archive = zip(valid());
            InputStream interrupted = new InputStream() {
                private int position;

                @Override
                public int read() throws IOException {
                    if (position >= archive.length / 2) throw new IOException("Import read interrupted");
                    return archive[position++] & 255;
                }

                @Override
                public int read(byte[] buffer, int offset, int length) throws IOException {
                    return super.read(buffer, offset, Math.min(1, length));
                }
            };
            try {
                GameDataImporter.importZip(interrupted, directory);
                throw new AssertionError("Interrupted import was accepted");
            } catch (IOException expected) {
                assert expected.getMessage().equals("Import read interrupted");
            }
            assert Arrays.equals(Files.readAllBytes(save), new byte[]{4, 5, 6});
            assert Arrays.equals(Files.readAllBytes(root.resolve("gamedata/BIN/TEST.BIN")), previousAsset);
            assert !root.resolve("gamedata-import").toFile().exists();
            System.out.println("Android game-data import checks passed");
        } finally {
            try (var paths = Files.walk(root)) {
                for (Path path : paths.sorted(java.util.Comparator.reverseOrder()).toList()) Files.delete(path);
            }
        }
    }
}
