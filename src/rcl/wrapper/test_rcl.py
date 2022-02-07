
import types
import rcl
import time

def read_exp(exp):
    return read_exp_context(rcl.ReadContext(exp))

def read_exp_context(c):
    etype = c.getType()
    if (etype == 'double'):
        return c.getDouble()
    elif (etype == 'long'):
        return c.getLong()
    elif (etype == 'bool'):
        return c.getBool()
    elif (etype == 'string'):
        return c.getString()
    elif (etype == 'vector'):
        return read_vector_context(c)
    elif (etype == 'map'):
        return read_map_context(c)
    else:
        raise "read_exp_context: unknown type "+etype+" in rcl expression"

def read_vector_context(c):
    vec = []
    c.vectorEnter()
    while not c.vectorIsAtEnd():
        vec.append(read_exp_context(c))
        c.vectorNextEntry()
    c.vectorExit()
    return vec

def read_map_context(c):
    m = {}
    c.mapEnter()
    while not c.mapIsAtEnd():
        m[c.mapGetFieldName()] = read_exp_context(c)
        c.mapNextEntry()
    c.mapExit()
    return m

def write_exp(pyexp):
    c = rcl.WriteContext()
    write_exp_context(c, pyexp)
    return c.getExp()

def write_exp_context(c, pyexp):
    etype = type(pyexp)
    if (etype is types.FloatType):
        c.setDouble(pyexp)
    elif (etype is types.IntType or etype is types.LongType):
        c.setLong(pyexp)
    elif (etype is types.StringType):
        c.setString(pyexp);
    elif (etype is types.ListType):
        write_vector_context(c, pyexp)
    elif (etype is types.DictType):
        write_map_context(c, pyexp)
    else:
        raise "write_exp_context: unknown type "+etype+" in python expression"

def write_vector_context(c, pyexp):
    c.setVector()
    c.vectorEnter()
    for entry in pyexp:
        write_exp_context(c, entry)
        c.vectorPushBackEntry()
    c.vectorExit()
    
def write_map_context(c, pyexp):
    c.setMap()
    c.mapEnter()
    for field_name,value in pyexp.items():
        write_exp_context(c, value);
        c.mapPushBackEntry(field_name)
    c.mapExit()

def simple_test():
    expr = rcl.readFromString("{a=>1, z=>[3.0, {c=>2, d=>3}, false], x=>0}")
    pyexp = read_exp(expr)
    print "pyexp = ", pyexp
    expr_out = write_exp(pyexp)
    print "expr_out = ", rcl.toStringCompact(expr_out)

def stress_test():
    f = file('example.config')
    x = ''
    for line in f:
        x = x + line
    startTime = time.time()
    nextReport = 10
    i = 1
    while 1:
        pyexp = read_exp(rcl.readFromString(x))
        if i == nextReport:
            print "after ", i, " iterations: ", (time.time() - startTime)/i, \
                  " seconds per parse"
            nextReport = nextReport*10
        i = i+1

#stress_test()
simple_test()
