module mod_a
contains
  subroutine foo(a, b)
    real(8), intent(in) :: a(:)
    real(8), intent(out) :: b(size(a))
    b = a * 2d0
  end subroutine
end module
