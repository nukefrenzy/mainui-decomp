// Decompiles every non-thunk, non-external function into one C file with markers.
// Usage (headless postScript): ExportDecomp.java <outfile.c>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;

public class ExportDecomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        String outFile = getScriptArgs()[0];
        DecompInterface di = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        di.setOptions(opts);
        di.toggleCCode(true);
        di.toggleSyntaxTree(false);
        di.setSimplificationStyle("decompile");
        if (!di.openProgram(currentProgram)) {
            throw new Exception("Decompiler failed to open program: " + di.getLastMessage());
        }
        int ok = 0, fail = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(outFile))) {
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                if (f.isThunk() || f.isExternal()) continue;
                if (monitor.isCancelled()) break;
                DecompileResults r = di.decompileFunction(f, 60, monitor);
                w.println("//@@FUNC " + f.getEntryPoint() + " " + f.getName(true));
                if (r != null && r.decompileCompleted() && r.getDecompiledFunction() != null) {
                    w.println(r.getDecompiledFunction().getC());
                    ok++;
                } else {
                    w.println("// decompile failed: " + (r == null ? "null" : r.getErrorMessage()));
                    fail++;
                }
            }
        }
        di.dispose();
        println("ExportDecomp done: ok=" + ok + " failed=" + fail);
    }
}
