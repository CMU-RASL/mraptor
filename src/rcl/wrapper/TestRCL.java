import java.lang.*;
import java.util.*;
//import rcl_java.*;

public class TestRCL
{
    static boolean debugPrint = true;

    static {
	// dynamically load librcl_java.so found in LD_LIBRARY_PATH
	System.loadLibrary("rcl_java");
    }

    public static Object read_exp(rcl_java.Exp expr) {
	return read_exp_context(new rcl_java.ReadContext(expr));
    }

    public static Object read_exp_context(rcl_java.ReadContext c) {
	String etype = c.getType();
	if (etype.equals("double")) {
	    if (debugPrint) {
		System.out.println("  double "+Double.toString(c.getDouble()));
	    }
	    return new Double(c.getDouble());
	} else if (etype.equals("long")) {
	    if (debugPrint) {
		System.out.println("  long "+Long.toString(c.getLong()));
	    }
	    return new Long(c.getLong());
	} else if (etype.equals("bool")) {
	    if (debugPrint) {
		System.out.println("  bool "+Boolean.toString(c.getBool()));
	    }
	    return new Boolean(c.getBool());
	} else if (etype.equals("string")) {
	    if (debugPrint) {
		System.out.println("  string "+c.getString());
	    }
	    return c.getString();
	} else if (etype.equals("vector")) {
	    return read_vector_context(c);
	} else if (etype.equals("map")) {
	    return read_map_context(c);
	} else {
	    throw new RuntimeException("RCL read_exp_context: unknown type '"+etype+"' in rcl expression");
	}
    }

    public static Object read_vector_context(rcl_java.ReadContext c) {
	Vector vec = new Vector();
	if (debugPrint) {
	    System.out.println("  entering vector");
	}
	c.vectorEnter();
	while (!c.vectorIsAtEnd()) {
	    vec.addElement(read_exp_context(c));
	    c.vectorNextEntry();
	}
	c.vectorExit();
	if (debugPrint) {
	    System.out.println("  exiting vector");
	}
	
	return vec;
    }

    public static Object read_map_context(rcl_java.ReadContext c) {
	HashMap m = new HashMap();

	if (debugPrint) {
	    System.out.println("  entering map");
	}
	c.mapEnter();
	while (!c.mapIsAtEnd()) {
	    if (debugPrint) {
		System.out.println("  map field "+c.mapGetFieldName());
	    }
	    m.put(c.mapGetFieldName(), read_exp_context(c));
	    c.mapNextEntry();
	}
	c.mapExit();
	if (debugPrint) {
	    System.out.println("  exiting map");
	}
	
	return m;
    }

    public static rcl_java.Exp write_exp(Object obj) {
	rcl_java.WriteContext c = new rcl_java.WriteContext();
	write_exp_context(c, obj);
	return c.getExp();
    }

    public static void write_exp_context(rcl_java.WriteContext c, Object obj) {
	String etype = obj.getClass().getName();
	if (etype.equals("java.lang.Double")) {
	    c.setDouble(((Double) obj).doubleValue());
	} else if (etype.equals("java.lang.Long")) {
	    c.setLong((int) ((Long) obj).longValue());
	} else if (etype.equals("java.lang.Boolean")) {
	    c.setBool(((Boolean) obj).booleanValue());
	} else if (etype.equals("java.lang.String")) {
	    c.setString((String) obj);
	} else if (etype.equals("java.util.Vector")) {
	    write_vector_context(c,obj);
	} else if (etype.equals("java.util.HashMap")) {
	    write_map_context(c,obj);
	} else {
	    throw new RuntimeException("RCL write_exp_context: unknown type '"+etype+"' in java expression");
	}
    }

    public static void write_vector_context(rcl_java.WriteContext c,
					    Object obj)
    {
	c.setVector();
	c.vectorEnter();
	Iterator i = ((Vector) obj).iterator();
	while (i.hasNext()) {
	    write_exp_context(c, i.next());
	    c.vectorPushBackEntry();
	}
	c.vectorExit();
    }

    public static void write_map_context(rcl_java.WriteContext c,
					 Object obj)
    {
	c.setMap();
	c.mapEnter();
	Iterator i = ((HashMap) obj).entrySet().iterator();
	while (i.hasNext()) {
	    Map.Entry entry = (Map.Entry) i.next();
	    write_exp_context(c, entry.getValue());
	    c.mapPushBackEntry((String) entry.getKey());
	}
	c.mapExit();
    }

    public static final void simpleTest() {
	rcl_java.Exp rcl_in = rcl_java.rcl.readFromString
	    ("{a=>1, z=>[3.5, {c=>2, d=>3}, false], x=>0}");
	System.out.println("rcl_in = "
			   + rcl_java.rcl.toStringCompact(rcl_in));

	System.out.println("");
	System.out.println("traversing rcl_in");
	Object java_expr = read_exp(rcl_in);

	rcl_java.Exp rcl_out = write_exp(java_expr);
	System.out.println("");
	System.out.println("rcl_out = "
			   + rcl_java.rcl.toStringCompact(rcl_out));
    }

    public static final void stressTest() {
	debugPrint = false;

	long startTime = System.currentTimeMillis();
	int nextReport = 10;
	for (int i=1; ; i++) {
	    Object java_expr =
		read_exp(rcl_java.rcl.readFromString
			 ("{a=>1, z=>[3.5, {c=>2, d=>3}, false], x=>0}"));
	    if (i == nextReport) {
		double timeDiff =
		    1e-3 * (System.currentTimeMillis() - startTime);
		System.out.println("after " + Integer.toString(i)
				   + " iterations: "
				   + Double.toString(timeDiff / i)
				   + " seconds per parse");
		nextReport = nextReport*10;
	    }
	}
    }

    public static final void main(String args[]) {
	simpleTest();
	//stressTest();
    }
} 
