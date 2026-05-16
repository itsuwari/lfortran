program c_backend_scalar_merge_inline_01
implicit none

integer :: a, b
real(8) :: x
logical :: m

a = 7
b = 3

m = .true.
if (merge(a, b, m) /= 7) error stop

m = .false.
if (merge(a, b, m) /= 3) error stop

x = merge(2.5d0, -1.5d0, a > b)
if (abs(x - 2.5d0) > 1.0d-12) error stop

end program
