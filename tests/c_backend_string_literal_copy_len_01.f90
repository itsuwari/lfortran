program c_backend_string_literal_copy_len_01
implicit none

character(len=4), allocatable :: sym(:)

allocate(sym(2), source='    ')

if (sym(1) /= '    ') error stop
if (sym(2) /= '    ') error stop

end program
