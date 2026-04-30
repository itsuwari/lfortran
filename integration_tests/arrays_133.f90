program arrays_133
    implicit none
    integer :: i
    real :: b(4), c(4), expected

    b = [-1.0, -2.0, 3.0, 4.0]
    c = [4.0, 9.0, 16.0, 25.0]
    expected = 24.0

    do i = 1, 5
        call check_sum(abs(b(:)) + sqrt(c(:)), expected)
    end do

contains

    subroutine check_sum(x, expected_sum)
        real, intent(in) :: x(4)
        real, intent(in) :: expected_sum

        if (abs(sum(x) - expected_sum) > 1.e-6) error stop
    end subroutine

end program
