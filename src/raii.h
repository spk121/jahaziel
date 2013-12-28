#ifndef JAHA_RAII_H
#define JAHA_RAII_H
#define RAII_VARIABLE(vartype,varname,initval,dtor) \
    void _dtor_ ## varname (vartype * v) { dtor(v); } \
    vartype varname __attribute__((cleanup(_dtor_ ## varname))) = (initval)
#endif
