program array_section_36
implicit none

real(8) :: grid(3, 2)

grid = -1.0d0
call fill_grid(grid)

if (abs(grid(1, 1) - 2.0d0) > 1.0d-12) error stop
if (abs(grid(2, 1)) > 1.0d-12) error stop
if (abs(grid(3, 1)) > 1.0d-12) error stop
if (abs(grid(1, 2) + 2.0d0) > 1.0d-12) error stop
if (abs(grid(2, 2)) > 1.0d-12) error stop
if (abs(grid(3, 2)) > 1.0d-12) error stop

contains

subroutine fill_grid(x)
    real(8), intent(inout) :: x(3, *)
    real(8) :: a, z

    a = 2.0d0
    z = 0.0d0
    x(:, 1) = [ a, z, z]
    x(:, 2) = [-a, z, z]
end subroutine fill_grid

end program array_section_36
