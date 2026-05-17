module c_backend_allocate_source_repeat_mod
implicit none

integer, parameter :: symbol_length = 4

end module

program c_backend_allocate_source_repeat_01
use c_backend_allocate_source_repeat_mod, only: symbol_length
implicit none

integer :: i
character(len=symbol_length), allocatable :: sym(:)

allocate(sym(3), source=repeat(' ', symbol_length))

do i = 1, 3
    if (sym(i) /= '    ') error stop
end do

end program
