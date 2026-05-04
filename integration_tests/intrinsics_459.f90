program intrinsics_459
implicit none

real(8) :: dist(5)
logical :: mask(5)
integer :: img, pos

dist = [10.0d0, 1.0d0, -99.0d0, -100.0d0, -101.0d0]
mask = [.true., .true., .true., .true., .true.]
img = 2

pos = minloc(dist(:img), dim=1)
if (pos /= 2) error stop 1

mask(2) = .false.
pos = minloc(dist(:img), dim=1, mask=mask(:img))
if (pos /= 1) error stop 2

end program intrinsics_459
