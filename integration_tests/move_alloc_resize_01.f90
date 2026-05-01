module move_alloc_resize_01_m
implicit none

interface resize
    module procedure resize_int
end interface

contains

subroutine resize_int(var, n)
    integer, allocatable, intent(inout) :: var(:)
    integer, intent(in), optional :: n
    integer, allocatable :: tmp(:)
    integer :: new_size, old_size

    if (allocated(var)) then
        old_size = size(var)
        call move_alloc(var, tmp)
    else
        old_size = 4
    end if

    if (present(n)) then
        new_size = n
    else
        new_size = old_size + old_size/2 + 1
    end if

    allocate(var(new_size))
    var = -99

    if (allocated(tmp)) then
        old_size = min(size(tmp), size(var))
        var(:old_size) = tmp(:old_size)
    end if
end subroutine

end module

program move_alloc_resize_01
use move_alloc_resize_01_m, only: resize
implicit none

integer, allocatable :: values(:)
integer :: i

call resize(values, 4)
do i = 1, 25
    if (size(values) < i) call resize(values)
    values(i) = i
end do

call resize(values, 25)

if (size(values) /= 25) error stop
if (sum(values) /= 325) error stop
if (values(1) /= 1) error stop
if (values(5) /= 5) error stop
if (values(25) /= 25) error stop

end program
