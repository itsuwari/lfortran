module c_backend_array_constant_item_static_index_01
implicit none

real(8), parameter :: table(3, 2) = reshape([ &
    1.0d0, 2.0d0, 3.0d0, &
    4.0d0, 5.0d0, 6.0d0], [3, 2])

contains

subroutine pick_value(y)
real(8), intent(out) :: y

y = table(2, 2)
end subroutine pick_value

end module c_backend_array_constant_item_static_index_01
