module c_backend_specialized_pass_array_no_copy_01_a
implicit none

type :: container
    real(8) :: bias
end type

contains

subroutine inner(mol, xyz, trans, total)
type(container), intent(in) :: mol
real(8), intent(in) :: xyz(:, :)
real(8), intent(in) :: trans(:, :)
real(8), intent(inout) :: total

total = total + mol%bias + xyz(2, 1) + trans(3, 1)

end subroutine

end module

module c_backend_specialized_pass_array_no_copy_01_b
use c_backend_specialized_pass_array_no_copy_01_a, only: container, inner
implicit none

contains

subroutine outer(mol, xyz, trans, total)
type(container), intent(in) :: mol
real(8), intent(in) :: xyz(:, :)
real(8), intent(in) :: trans(:, :)
real(8), intent(inout) :: total

call inner(mol, xyz, trans, total)

end subroutine

end module

module c_backend_specialized_pass_array_no_copy_01_c
implicit none

type :: downloader
contains
    procedure, nopass :: upload_form
end type

contains

subroutine upload_form(form_data)
integer, intent(inout) :: form_data(:)
integer :: i

do i = 1, size(form_data)
    form_data(i) = i
end do

end subroutine

end module

program c_backend_specialized_pass_array_no_copy_01
use c_backend_specialized_pass_array_no_copy_01_a, only: container
use c_backend_specialized_pass_array_no_copy_01_b, only: outer
use c_backend_specialized_pass_array_no_copy_01_c, only: downloader
implicit none

type(container) :: mol
type(downloader) :: d
real(8) :: xyz(3, 2), trans(3, 1), aderiv(3, 3), result(3), total
real(8) :: x1, x2, x3, x4, x5, x6, x7, x8, x9
real(8) :: scale1, scale2
integer :: form(5)

mol%bias = 1.0d0
xyz = reshape([10.0d0, 20.0d0, 30.0d0, 40.0d0, 50.0d0, 60.0d0], [3, 2])
trans(:, 1) = [100.0d0, 200.0d0, 300.0d0]
x1 = 1.0d0
x2 = 2.0d0
x3 = 3.0d0
x4 = 4.0d0
x5 = 5.0d0
x6 = 6.0d0
x7 = 7.0d0
x8 = 8.0d0
x9 = 9.0d0
scale1 = 2.0d0
scale2 = -0.25d0
aderiv(:, :) = reshape([x1, x2, x3, x4, x5, x6, x7, x8, x9], &
    shape=[3, 3]) * scale1 * scale2
result = matmul(aderiv, [2.0d0, -1.0d0, 0.5d0])
total = 0.0d0

call outer(mol, xyz, trans, total)
call d%upload_form(form)

if (abs(total - 321.0d0) > 1.0d-12) error stop
if (abs(result(1) + 0.75d0) > 1.0d-12) error stop
if (abs(result(2) + 1.50d0) > 1.0d-12) error stop
if (abs(result(3) + 2.25d0) > 1.0d-12) error stop
if (form(1) /= 1 .or. form(2) /= 2 .or. form(3) /= 3) error stop
if (form(4) /= 4 .or. form(5) /= 5) error stop

end program
