// Decompile listed functions after renaming. Usage: ExportFew.java <out.c> addr...
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
public class ExportFew extends GhidraScript {
  public void run() throws Exception {
    String[] a = getScriptArgs();
    DecompInterface di = new DecompInterface(); di.openProgram(currentProgram);
    try (PrintWriter w = new PrintWriter(new FileWriter(a[0]))) {
      for (int i = 1; i < a.length; i++) {
        Function f = getFunctionAt(toAddr(Long.parseLong(a[i], 16)));
        w.println(di.decompileFunction(f, 60, monitor).getDecompiledFunction().getC());
      }
    }
  }
}
