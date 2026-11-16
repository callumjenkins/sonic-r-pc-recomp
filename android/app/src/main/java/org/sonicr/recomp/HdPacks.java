package org.sonicr.recomp;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

final class HdPacks {
    private HdPacks() {}

    /** The packs in a game data's HD folder that the engine accepts, sorted. */
    static List<String> list(File hd) {
        List<String> packs = new ArrayList<>();
        File[] folders = hd.listFiles(File::isDirectory);
        if (folders == null) return packs;
        for (File folder : folders) if (accepted(folder.getName())) packs.add(folder.getName());
        packs.sort(String.CASE_INSENSITIVE_ORDER);
        return packs;
    }

    static String choose(List<String> packs, String preferred) {
        if (packs.contains(preferred)) return preferred;
        return packs.isEmpty() ? "" : packs.get(0);
    }

    /** The pack to remember after `picked` is selected. A missing remembered pack stays until a pack other than the
     *  stand-in `choose` gives is picked. */
    static String remembered(List<String> packs, String preferred, String picked) {
        return picked.equals(choose(packs, preferred)) ? preferred : picked;
    }

    /** The engine's rule, which also has to hold for the modified UTF-8 that JNI hands it, so surrogates are out. */
    private static boolean accepted(String name) {
        if (name.isEmpty() || name.startsWith(".") || name.contains("\\")) return false;
        if (name.getBytes(StandardCharsets.UTF_8).length >= 64) return false;
        for (char c : name.toCharArray()) if (c < 0x20 || c == 0x7f || Character.isSurrogate(c)) return false;
        return true;
    }
}
