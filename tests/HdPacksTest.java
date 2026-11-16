package org.sonicr.recomp;

import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.List;

public final class HdPacksTest {
    public static void main(String[] args) throws Exception {
        File hd = Files.createTempDirectory("sonicr-hd-packs").toFile();
        assert HdPacks.list(new File(hd, "missing")).isEmpty();
        assert HdPacks.list(hd).isEmpty();
        String[] names = {"Zed", "apple", ".hidden", "a\tb", "a\\b", "x\u007f", "Pack \uD83D\uDE00",
            "\u00e9".repeat(32), "\u00e9".repeat(31)};
        for (String name : names) assert new File(hd, name).mkdir() : name;
        assert new File(hd, "NOTES.TXT").createNewFile();
        List<String> packs = HdPacks.list(hd);
        assert packs.equals(Arrays.asList("apple", "Zed", "\u00e9".repeat(31))) : packs;
        assert HdPacks.choose(packs, "Zed").equals("Zed");
        assert HdPacks.choose(packs, "Gone").equals("apple");
        assert HdPacks.choose(List.of(), "Gone").isEmpty();
        assert HdPacks.remembered(packs, "Gone", "apple").equals("Gone");
        assert HdPacks.remembered(packs, "Gone", "Zed").equals("Zed");
        assert HdPacks.remembered(packs, "apple", "Zed").equals("Zed");
        System.out.println("HD pack list checks passed");
    }
}
