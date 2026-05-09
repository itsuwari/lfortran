program c_backend_rank2_nonself_section_copy_01
implicit none

real(8) :: a(3, 5), b(3, 4), c(3, 3), d(3, 5)
integer :: i, j

do j = 1, 5
    do i = 1, 3
        a(i, j) = 100.0d0 * j + i
    end do
end do

b = -1.0d0
call copy_suffix(a, b)
do j = 1, 4
    do i = 1, 3
        if (abs(b(i, j) - (100.0d0 * (j + 1) + i)) > 1.0d-12) error stop
    end do
end do

c = -2.0d0
call copy_middle(a, c)
do j = 1, 3
    do i = 1, 3
        if (abs(c(i, j) - (100.0d0 * (j + 1) + i)) > 1.0d-12) error stop
    end do
end do

d = -3.0d0
call copy_whole(a, d)
do j = 1, 5
    do i = 1, 3
        if (abs(d(i, j) - a(i, j)) > 1.0d-12) error stop
    end do
end do

contains

subroutine copy_suffix(src, dst)
    real(8), intent(in) :: src(:, :)
    real(8), intent(out) :: dst(:, :)

    dst = src(:, 2:)
end subroutine

subroutine copy_middle(src, dst)
    real(8), intent(in) :: src(:, :)
    real(8), intent(out) :: dst(:, :)

    dst = src(:, 2:4)
end subroutine

subroutine copy_whole(src, dst)
    real(8), intent(in) :: src(:, :)
    real(8), intent(out) :: dst(:, :)

    dst = src
end subroutine

end program
