program c_backend_inline_logical_reduction_01
implicit none

integer :: a(4)
logical :: mask(2, 2)
real(8) :: charge
integer :: uhf

a = [1, 2, 3, 4]
mask = reshape([.true., .false., .true., .true.], [2, 2])
charge = 1.25_8
uhf = 0

if (.not. any(a == 3)) error stop
if (any(a < 0)) error stop
if (.not. all(a > 0)) error stop
if (all(a /= 3)) error stop
if (count(a > 2) /= 2) error stop
if (.not. any([nint(charge), uhf] /= 0)) error stop
if (merge(2, 1, a(3) == 3 .and. any(a == [0, 2, 5, 7])) /= 2) error stop

if (.not. any(mask)) error stop
if (all(mask)) error stop
if (count(mask) /= 3) error stop

end program c_backend_inline_logical_reduction_01
