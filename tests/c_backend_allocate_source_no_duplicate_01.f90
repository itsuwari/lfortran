program c_backend_allocate_source_no_duplicate_01
implicit none

integer :: n
real, allocatable :: x(:), y(:, :)

n = 10
allocate(x(n), source=0.0)
allocate(y(2, n), source=0.0)

if (sum(x) /= 0.0) error stop
if (sum(y) /= 0.0) error stop

end program
