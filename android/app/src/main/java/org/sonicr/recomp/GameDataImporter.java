package org.sonicr.recomp;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public final class GameDataImporter {
    private static final Set<String> REQUIRED = new HashSet<>(Arrays.asList(
        "AI", "BIN", "CITY", "EMERALD", "FACTORY", "GENERAL", "ISLAND", "RUIN", "SOUND"));
    /* Imported when present, and replaced as a whole by every import. */
    private static final Set<String> OPTIONAL = new HashSet<>(Arrays.asList("HD", "MUSIC"));

    private GameDataImporter() {}

    public static boolean isReady(File directory) {
        for (String folder : REQUIRED) {
            if (!hasFiles(new File(directory, folder))) return false;
        }
        return true;
    }

    private static boolean hasFiles(File directory) {
        File[] files = directory.listFiles();
        if (files == null) return false;
        for (File file : files) {
            if (file.isFile() || hasFiles(file)) return true;
        }
        return false;
    }

    public static synchronized void recover(File destination) throws IOException {
        File backup = new File(destination.getParentFile(), "gamedata-previous");
        if (!destination.exists() && backup.exists() && !backup.renameTo(destination)) {
            throw new IOException("Cannot restore the previous game data.");
        }
    }

    public static synchronized void importZip(InputStream input, File destination) throws IOException {
        recover(destination);
        File staging = new File(destination.getParentFile(), "gamedata-import");
        File backup = new File(destination.getParentFile(), "gamedata-previous");
        delete(staging);
        if (!staging.mkdirs()) throw new IOException("Cannot prepare game data folder.");
        try {
            extract(input, staging);
            if (!isReady(staging)) throw new IOException("The ZIP is missing required game folders.");
            File[] previous = destination.listFiles();
            if (previous != null) {
                for (File file : previous) {
                    if (!REQUIRED.contains(file.getName()) && !OPTIONAL.contains(file.getName())) {
                        copy(file, new File(staging, file.getName()));
                    }
                }
            }
            delete(backup);
            boolean moved = destination.exists();
            if (moved && !destination.renameTo(backup)) throw new IOException("Cannot replace game data.");
            if (!staging.renameTo(destination)) {
                if (moved && !backup.renameTo(destination)) {
                    throw new IOException("Import failed. Previous data is in gamedata-previous.");
                }
                throw new IOException("Cannot install imported game data.");
            }
            delete(backup);
        } finally {
            delete(staging);
        }
    }

    private static void extract(InputStream input, File directory) throws IOException {
        Set<String> files = new HashSet<>();
        long total = 0;
        int entries = 0;
        byte[] buffer = new byte[65536];
        try (ZipInputStream zip = new ZipInputStream(input)) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (++entries > 10000) throw new IOException("The ZIP contains too many entries.");
                String name = entry.getName().replace('\\', '/');
                if (name.startsWith("/")) throw new IOException("The ZIP contains an absolute path.");
                for (String part : name.split("/")) {
                    if (part.equals("..") || part.equals(".")) throw new IOException("The ZIP contains an invalid path.");
                }
                String[] parts = name.split("/", 2);
                String folder = parts[0].toUpperCase(Locale.ROOT);
                if (!REQUIRED.contains(folder) && !OPTIONAL.contains(folder)) continue;
                String relative = folder;
                if (parts.length == 2) relative += "/" + gamePath(folder, parts[1]);
                File target = new File(directory, relative);
                if (!target.getCanonicalPath().startsWith(directory.getCanonicalPath() + File.separator)) {
                    throw new IOException("The ZIP contains an invalid path.");
                }
                if (entry.isDirectory()) {
                    if (!target.isDirectory() && !target.mkdirs()) throw new IOException("Cannot create a game folder.");
                    continue;
                }
                if (!files.add(target.getCanonicalPath())) throw new IOException("The ZIP contains duplicate game files.");
                File parent = target.getParentFile();
                if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("Cannot create a game folder.");
                try (FileOutputStream output = new FileOutputStream(target)) {
                    int size;
                    while ((size = zip.read(buffer)) != -1) {
                        total = checkedTotal(total, size);
                        output.write(buffer, 0, size);
                    }
                }
            }
        }
    }

    static long checkedTotal(long total, int size) throws IOException {
        long result = total + size;
        if (result > 1024L * 1024 * 1024) throw new IOException("The game data exceeds 1 GB.");
        return result;
    }

    private static void copy(File source, File destination) throws IOException {
        if (source.isDirectory()) {
            if (!destination.mkdirs()) throw new IOException("Cannot preserve a save folder.");
            File[] files = source.listFiles();
            if (files == null) throw new IOException("Cannot read a save folder.");
            for (File file : files) copy(file, new File(destination, file.getName()));
        } else {
            try (FileInputStream input = new FileInputStream(source);
                 FileOutputStream output = new FileOutputStream(destination)) {
                byte[] buffer = new byte[65536];
                int size;
                while ((size = input.read(buffer)) != -1) output.write(buffer, 0, size);
            }
        }
    }

    private static void delete(File file) throws IOException {
        if (!file.exists()) return;
        if (file.isDirectory()) {
            File[] files = file.listFiles();
            if (files == null) throw new IOException("Cannot read temporary game data.");
            for (File child : files) delete(child);
        }
        if (!file.delete()) throw new IOException("Cannot remove temporary game data.");
    }

    /** The engine's case for a path inside `folder`. A pack's folder keeps its own case, because its name is shown. */
    private static String gamePath(String folder, String path) {
        if (folder.equals("MUSIC")) return path.toLowerCase(Locale.ROOT);
        if (!folder.equals("HD")) return path.toUpperCase(Locale.ROOT);
        String[] parts = path.split("/", 2);
        return parts.length == 2 ? parts[0] + "/" + parts[1].toUpperCase(Locale.ROOT) : parts[0];
    }
}
