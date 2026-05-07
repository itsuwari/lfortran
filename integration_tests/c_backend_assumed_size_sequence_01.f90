program c_backend_assumed_size_sequence_01
   implicit none

   real :: weights(8)
   real :: grid(3, 4)
   integer :: start

   weights = 0.0
   grid = 0.0

   start = 4
   call fill_weights(weights(start))
   if (weights(1) /= 0.0 .or. weights(2) /= 0.0 .or. weights(3) /= 0.0) error stop
   if (weights(4) /= 1.0 .or. weights(5) /= 2.0 .or. weights(6) /= 3.0) error stop
   if (weights(7) /= 0.0 .or. weights(8) /= 0.0) error stop

   call fill_grid(grid(1, 2))
   if (grid(1, 1) /= 0.0 .or. grid(2, 1) /= 0.0 .or. grid(3, 1) /= 0.0) error stop
   if (grid(1, 2) /= 1.0 .or. grid(2, 2) /= 2.0 .or. grid(3, 2) /= 3.0) error stop
   if (grid(1, 3) /= 4.0 .or. grid(2, 3) /= 5.0 .or. grid(3, 3) /= 6.0) error stop
   if (grid(1, 4) /= 0.0 .or. grid(2, 4) /= 0.0 .or. grid(3, 4) /= 0.0) error stop

contains

   subroutine fill_weights(w)
      real, intent(inout) :: w(*)

      w(1:3) = [1.0, 2.0, 3.0]
   end subroutine fill_weights

   subroutine fill_grid(x)
      real, intent(inout) :: x(3, *)
      real :: a, b, c

      a = 1.0
      b = 2.0
      c = 3.0
      x(:, 1) = [a, b, c]
      x(:, 2) = [a + 3.0, b + 3.0, c + 3.0]
   end subroutine fill_grid

end program c_backend_assumed_size_sequence_01
