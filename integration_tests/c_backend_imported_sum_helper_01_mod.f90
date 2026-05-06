module c_backend_imported_sum_helper_01_mod
implicit none
contains

subroutine imported_sum(a, b)
    real(8), intent(in) :: a(:, :)
    real(8), intent(out) :: b(:)

    b = sum(a, 1)
end subroutine

end module
