// Applies MainUI names, class namespaces, comments and global labels from names.tsv / globals.tsv.
// Run from Ghidra's Script Manager (asks for the folder) or headless: ApplyMainUINames.java <folder>
// Only renames functions still carrying default (FUN_) names or names set by this script,
// so your own renames are kept. Re-running is safe.
//@category MainUI
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ApplyMainUINames extends GhidraScript {
    private static final String TAG = "[MainUI map]";

    private Namespace classNs(String name) throws Exception {
        SymbolTable st = currentProgram.getSymbolTable();
        Namespace ns = st.getNamespace(name, currentProgram.getGlobalNamespace());
        if (ns == null) {
            ns = st.createClass(currentProgram.getGlobalNamespace(), name, SourceType.IMPORTED);
        }
        return ns;
    }

    @Override
    public void run() throws Exception {
        String dir = getScriptArgs().length > 0 ? getScriptArgs()[0]
            : askDirectory("Folder containing names.tsv", "Select").getAbsolutePath();
        FunctionManager fm = currentProgram.getFunctionManager();
        int renamed = 0, skipped = 0, missing = 0, created = 0;

        for (String line : Files.readAllLines(Paths.get(dir, "names.tsv"))) {
            if (line.startsWith("addr\t") || line.isBlank()) continue;
            String[] f = line.split("\t", -1);
            Address a = toAddr(Long.parseLong(f[0], 16));
            Function fn = fm.getFunctionAt(a);
            if (fn == null) {
                // Methods reached only through vtables are often not discovered by auto-analysis
                if (getInstructionAt(a) == null) disassemble(a);
                fn = createFunction(a, null);
                if (fn == null) { missing++; continue; }
                created++;
            }
            String cur = fn.getName();
            String plate = fn.getComment();
            boolean ours = plate != null && plate.contains(TAG);
            if (!cur.startsWith("FUN_") && !ours) { skipped++; continue; }

            String name = f[1];
            Namespace ns = currentProgram.getGlobalNamespace();
            int sep = name.lastIndexOf("::");
            if (sep > 0) {
                ns = classNs(name.substring(0, sep));
                name = name.substring(sep + 2);
            }
            name = name.replaceAll("[^A-Za-z0-9_~]", "_");
            fn.setParentNamespace(ns);
            fn.setName(name, SourceType.USER_DEFINED);
            StringBuilder c = new StringBuilder(TAG + " subsystem=" + f[2] + " confidence=" + f[3]);
            if (f.length > 4 && !f[4].isEmpty()) c.append("\n").append(f[4]);
            fn.setComment(c.toString());
            renamed++;
        }

        int labels = 0;
        Path g = Paths.get(dir, "globals.tsv");
        if (Files.exists(g)) {
            for (String line : Files.readAllLines(g)) {
                if (line.startsWith("addr\t") || line.isBlank()) continue;
                String[] f = line.split("\t", -1);
                Address a = toAddr(Long.parseLong(f[0], 16));
                currentProgram.getSymbolTable().createLabel(a, f[1], SourceType.USER_DEFINED);
                if (f.length > 2 && !f[2].isEmpty()) {
                    currentProgram.getListing().setComment(a, CommentType.EOL, TAG + " " + f[2]);
                }
                labels++;
            }
        }
        println(String.format("ApplyMainUINames: renamed=%d created=%d kept-your-names=%d no-function=%d globals=%d",
            renamed, created, skipped, missing, labels));
    }
}
