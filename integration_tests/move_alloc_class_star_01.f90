module move_alloc_class_star_01_m
implicit none

type :: cache_type
    real(8), allocatable :: values(:)
end type

type :: box_type
    class(*), allocatable :: raw
end type

contains

subroutine fill_box(box)
type(box_type), intent(inout) :: box
type(cache_type), allocatable :: tmp

if (.not. allocated(box%raw)) then
    allocate(tmp)
    allocate(tmp%values(2))
    tmp%values = [1.0d0, 2.0d0]
    call move_alloc(tmp, box%raw)
end if
end subroutine

subroutine check_box(box)
type(box_type), intent(inout) :: box

select type (ptr => box%raw)
type is (cache_type)
    if (.not. allocated(ptr%values)) error stop
    if (size(ptr%values) /= 2) error stop
    if (abs(ptr%values(2) - 2.0d0) > 1.0d-12) error stop
class default
    error stop
end select
end subroutine

end module

program main
use move_alloc_class_star_01_m, only: box_type, fill_box, check_box
implicit none

type(box_type) :: box

call fill_box(box)
call check_box(box)
end program
