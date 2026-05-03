module c_backend_derived_dummy_restrict_01_m
implicit none

type :: box_t
    real(8) :: x
end type

contains

subroutine read_box(box, y)
type(box_t), intent(in) :: box
real(8), intent(out) :: y

y = box%x

end subroutine

end module

program c_backend_derived_dummy_restrict_01
use c_backend_derived_dummy_restrict_01_m, only: box_t, read_box
implicit none

type(box_t) :: box
real(8) :: y

box%x = 4.25d0
call read_box(box, y)

if (abs(y - 4.25d0) > 1.0d-12) error stop

end program
