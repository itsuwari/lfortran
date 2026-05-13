program c_backend_split_helper_reachability_01
implicit none

type(_lfortran_list(integer)) :: x = _lfortran_list_constant(1, 2)

call _lfortran_list_append(x, 3)
if (_lfortran_len(x) /= 3) error stop
if (_lfortran_get_item(x, 2) /= 3) error stop

end program
