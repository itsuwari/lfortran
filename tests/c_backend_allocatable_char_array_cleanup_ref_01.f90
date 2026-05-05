program c_backend_allocatable_char_array_cleanup_ref_01
implicit none

character(len=4), allocatable :: sym(:)

allocate(sym(2))
sym = "    "
sym(1) = "Mn"
sym(2) = "O"
deallocate(sym)

end program
