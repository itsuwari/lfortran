subroutine c_backend_descriptor_cache_array_item_01(x, y)
real(8), intent(inout) :: x(3, *)
real(8), intent(out) :: y

x(1, 1) = 1.0d0
x(2, 1) = 2.0d0
y = x(2, 1)
end subroutine c_backend_descriptor_cache_array_item_01
