subroutine raw_sum(a, lda, n, res)
    integer, intent(in) :: lda, n
    real(8), intent(in) :: a(lda, *)
    real(8), intent(out) :: res
    integer :: i, j

    res = 0.0d0
    do j = 1, n
        do i = 1, lda
            res = res + dble(i + 100*j) * a(i, j)
        end do
    end do
end subroutine

program main
    real(8) :: x(2, 3, 2), r
    integer :: i, j

    x = 0.0d0
    do j = 1, 3
        do i = 1, 2
            x(i, j, 1) = dble(i + 10*j)
            x(i, j, 2) = dble(i + 1000*j)
        end do
    end do

    call pass_section(x, r)
    if (abs(r - 2819815.0d0) > 1.0d-8) error stop
    print *, r
contains
    subroutine pass_section(y, res)
        real(8), intent(in) :: y(:, :, :)
        real(8), intent(out) :: res

        call raw_sum(y(:, :, 2), size(y, 1), size(y, 2), res)
    end subroutine
end program
