// Exports functions, call graph, string references and import usage to TSV files.
// Usage (headless postScript): ExportStructure.java <outdir>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.address.*;
import ghidra.program.model.data.StringDataInstance;
import java.io.*;
import java.util.*;

public class ExportStructure extends GhidraScript {
    private String clean(String s) {
        return s.replace("\t", "\\t").replace("\n", "\\n").replace("\r", "\\r");
    }

    @Override
    public void run() throws Exception {
        String out = getScriptArgs()[0];
        new File(out).mkdirs();
        Listing listing = currentProgram.getListing();
        FunctionManager fm = currentProgram.getFunctionManager();
        ReferenceManager rm = currentProgram.getReferenceManager();

        // functions.tsv: entry, name, namespace, size, thunk/external flags
        try (PrintWriter fw = new PrintWriter(new FileWriter(out + "/functions.tsv"));
             PrintWriter cw = new PrintWriter(new FileWriter(out + "/calls.tsv"))) {
            fw.println("entry\tname\tnamespace\tsize\tis_thunk\tis_external\tthunk_target\tsignature");
            cw.println("caller\tcallee\tcallee_name");
            for (Function f : fm.getFunctions(true)) {
                String thunkTarget = "";
                if (f.isThunk() && f.getThunkedFunction(true) != null) {
                    thunkTarget = f.getThunkedFunction(true).getName();
                }
                fw.println(f.getEntryPoint() + "\t" + clean(f.getName()) + "\t" +
                    clean(f.getParentNamespace().getName(true)) + "\t" + f.getBody().getNumAddresses() + "\t" +
                    f.isThunk() + "\t" + f.isExternal() + "\t" + clean(thunkTarget) + "\t" +
                    clean(f.getSignature().getPrototypeString()));
                for (Function callee : f.getCalledFunctions(monitor)) {
                    Function real = callee.isThunk() && callee.getThunkedFunction(true) != null
                        ? callee.getThunkedFunction(true) : callee;
                    cw.println(f.getEntryPoint() + "\t" + callee.getEntryPoint() + "\t" + clean(real.getName()));
                }
            }
        }

        // strings.tsv: string address, value, referencing function entry
        try (PrintWriter sw = new PrintWriter(new FileWriter(out + "/strings.tsv"))) {
            sw.println("str_addr\tfrom_addr\tfunc_entry\tfunc_name\tvalue");
            DataIterator it = listing.getDefinedData(true);
            while (it.hasNext()) {
                Data d = it.next();
                if (!d.hasStringValue()) continue;
                String val = StringDataInstance.getStringDataInstance(d).getStringValue();
                if (val == null) continue;
                ReferenceIterator refs = rm.getReferencesTo(d.getAddress());
                boolean any = false;
                while (refs.hasNext()) {
                    Reference r = refs.next();
                    Function f = fm.getFunctionContaining(r.getFromAddress());
                    sw.println(d.getAddress() + "\t" + r.getFromAddress() + "\t" +
                        (f == null ? "" : f.getEntryPoint()) + "\t" + (f == null ? "" : clean(f.getName())) + "\t" + clean(val));
                    any = true;
                }
                if (!any) sw.println(d.getAddress() + "\t\t\t\t" + clean(val));
            }
        }

        // symbols.tsv: every non-default symbol with namespace (RTTI, vtables, labels)
        try (PrintWriter yw = new PrintWriter(new FileWriter(out + "/symbols.tsv"))) {
            yw.println("addr\tname\tnamespace\ttype\tsource");
            SymbolIterator si = currentProgram.getSymbolTable().getAllSymbols(true);
            while (si.hasNext()) {
                Symbol s = si.next();
                if (s.getSource() == SourceType.DEFAULT) continue;
                yw.println(s.getAddress() + "\t" + clean(s.getName()) + "\t" +
                    clean(s.getParentNamespace().getName(true)) + "\t" + s.getSymbolType() + "\t" + s.getSource());
            }
        }
        println("ExportStructure done: " + fm.getFunctionCount() + " functions");
    }
}
