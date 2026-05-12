module c_backend_no_copy_view_length_01_m
contains
    subroutine sum_assumed(x, s)
        real(8), intent(in) :: x(:, :)
        real(8), intent(out) :: s
        s = sum(x)
    end subroutine
end module

program c_backend_no_copy_view_length_01
    use c_backend_no_copy_view_length_01_m
    real(8) :: a(4, 5), s
    a = 1.0d0
    call sum_assumed(a(:, 2:4), s)
    if (abs(s - 12.0d0) > 1.0d-12) error stop
end program
