program c_backend_rank2_section_self_update_no_temp_01
implicit none

real(8) :: x(3, 2, 3, 2), y(3, 3), z(3, 3), scale
integer :: i, j

x = 1.0d0
scale = 4.0d0

do j = 1, 3
    do i = 1, 3
        y(i, j) = dble(i + 10*j)
        z(i, j) = dble(100 + i + 10*j)
    end do
end do

x(:, 1, :, 2) = x(:, 1, :, 2) + scale*(y + z)

if (abs(x(2, 1, 3, 2) - (1.0d0 + 4.0d0*((2.0d0 + 30.0d0) + (100.0d0 + 2.0d0 + 30.0d0)))) > 1.0d-12) error stop
if (abs(x(2, 2, 3, 2) - 1.0d0) > 1.0d-12) error stop

end program c_backend_rank2_section_self_update_no_temp_01
