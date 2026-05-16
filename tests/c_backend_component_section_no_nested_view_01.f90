module c_backend_component_section_no_nested_view_01_m
implicit none

type :: box_t
    real(8), allocatable :: a(:, :, :)
end type

contains

subroutine consume(x, total)
real(8), intent(in) :: x(:)
real(8), intent(out) :: total

total = sum(x)
end subroutine

subroutine run()
type(box_t) :: box
integer :: i, j, k
real(8) :: total, dot
real(8) :: weights(3)

allocate(box%a(3, 2, 2))

do k = 1, 2
    do j = 1, 2
        do i = 1, 3
            box%a(i, j, k) = real(i + 10*j + 100*k, 8)
        end do
    end do
end do

weights = [1.0d0, -2.0d0, 3.0d0]

call consume(box%a(:, 2, 1), total)
dot = dot_product(box%a(:, 2, 1), weights)

if (abs(total - (121.0d0 + 122.0d0 + 123.0d0)) > 1.0d-12) error stop
if (abs(dot - (121.0d0 - 2.0d0*122.0d0 + 3.0d0*123.0d0)) > 1.0d-12) error stop
end subroutine

end module

program c_backend_component_section_no_nested_view_01
use c_backend_component_section_no_nested_view_01_m, only: run
implicit none

call run()
end program
