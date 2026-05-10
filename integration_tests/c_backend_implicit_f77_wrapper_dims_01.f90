subroutine use_matrix(a, lda, info)
    integer, intent(in) :: lda
    integer, intent(out) :: info
    double precision, intent(inout) :: a(lda, *)

    a(1, 1) = a(1, 1) + 1.0d0
    info = lda
end subroutine

program main
    integer :: info
    double precision :: a(2, 2)

    a = 0.0d0
    call use_matrix(a, 2, info)
    if (info /= 2) error stop
    if (a(1, 1) /= 1.0d0) error stop
end program
