#include "type_generated.h"

void __lfortran_tblite_wavefunction_type__get_alpha_beta_occupation(double nocc, double nuhf, double *nalp, double *nbet)
{
    double diff;
    double ntmp;
    diff = fmin(nuhf, nocc);
    ntmp = nocc - diff;
    (*nalp) = ntmp/(double)(2) + diff;
    (*nbet) = ntmp/(double)(2);
}

