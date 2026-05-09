program c_backend_rank1_nonself_section_copy_01
implicit none

real(8) :: a(6), b(4), c(3), d(6), e(3), f(4)
real(8) :: total
integer :: i

do i = 1, 6
    a(i) = 10.0d0 * i
end do

b = -1.0d0
call copy_suffix(a, b)
do i = 1, 4
    if (abs(b(i) - 10.0d0 * (i + 2)) > 1.0d-12) error stop
end do

c = -2.0d0
call copy_middle(a, c)
do i = 1, 3
    if (abs(c(i) - 10.0d0 * (i + 1)) > 1.0d-12) error stop
end do

d = -3.0d0
call copy_whole(a, d)
do i = 1, 6
    if (abs(d(i) - a(i)) > 1.0d-12) error stop
end do

e = -4.0d0
call copy_fixed_column(e)
do i = 1, 3
    if (abs(e(i) - (100.0d0 + i + 10.0d0 * 2)) > 1.0d-12) error stop
end do

f = -5.0d0
call copy_fixed_row(f)
do i = 1, 4
    if (abs(f(i) - (100.0d0 + 2.0d0 + 10.0d0 * i)) > 1.0d-12) error stop
end do

call copy_fixed_column_local(total)
if (abs(total - ((100.0d0 + 1.0d0 + 20.0d0) + &
        (100.0d0 + 2.0d0 + 20.0d0) + &
        (100.0d0 + 3.0d0 + 20.0d0))) > 1.0d-12) error stop

contains

subroutine copy_suffix(src, dst)
    real(8), intent(in) :: src(:)
    real(8), intent(out) :: dst(:)

    dst = src(3:)
end subroutine

subroutine copy_middle(src, dst)
    real(8), intent(in) :: src(:)
    real(8), intent(out) :: dst(:)

    dst = src(2:4)
end subroutine

subroutine copy_whole(src, dst)
    real(8), intent(in) :: src(:)
    real(8), intent(out) :: dst(:)

    dst = src
end subroutine

subroutine copy_fixed_column(dst)
    real(8), intent(out) :: dst(3)
    real(8) :: src(3, 4)
    integer :: i, j

    do j = 1, 4
        do i = 1, 3
            src(i, j) = 100.0d0 + i + 10.0d0 * j
        end do
    end do
    dst = src(:, 2)
end subroutine

subroutine copy_fixed_row(dst)
    real(8), intent(out) :: dst(4)
    real(8) :: src(3, 4)
    integer :: i, j

    do j = 1, 4
        do i = 1, 3
            src(i, j) = 100.0d0 + i + 10.0d0 * j
        end do
    end do
    dst = src(2, :)
end subroutine

subroutine copy_fixed_column_local(total)
    real(8), intent(out) :: total
    real(8) :: src(3, 4), dst(3)
    integer :: i, j

    do j = 1, 4
        do i = 1, 3
            src(i, j) = 100.0d0 + i + 10.0d0 * j
        end do
    end do
    dst = src(:, 2)
    total = sum(dst)
end subroutine

end program
