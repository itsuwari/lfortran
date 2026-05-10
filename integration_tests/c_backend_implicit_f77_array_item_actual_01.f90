double precision function first_from(x)
    double precision :: x(*)
    first_from = x(1) + x(2)
end function

subroutine caller(a, lda, k, got)
    integer :: lda, k
    double precision :: a(lda, *), got, first_from
    external first_from

    got = first_from(a(max(1, k), 1))
end subroutine

program main
    integer :: k
    double precision :: a(2, 2), got

    k = 2
    a(1, 1) = 1.0d0
    a(2, 1) = 2.0d0
    a(1, 2) = 3.0d0
    a(2, 2) = 4.0d0

    call caller(a, 2, k, got)
    if (got /= 5.0d0) error stop
end program
