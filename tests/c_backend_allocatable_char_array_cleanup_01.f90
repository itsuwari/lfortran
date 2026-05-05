program c_backend_allocatable_char_array_cleanup_01
implicit none

call check_symbols()

contains

subroutine check_symbols()
    character(len=4), allocatable :: sym(:)

    allocate(sym(3))
    sym = "    "
    sym(1) = "Mn"
    sym(2) = "O"
    sym(3) = "O"

    if (trim(sym(1)) /= "Mn") error stop
    if (trim(sym(2)) /= "O") error stop
    if (trim(sym(3)) /= "O") error stop

    deallocate(sym)

    allocate(sym(1))
    sym(1) = "Fe"
    if (trim(sym(1)) /= "Fe") error stop
end subroutine

end program
