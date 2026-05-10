program c_backend_cleanup_dedupe_main_01
use c_backend_cleanup_dedupe_module_01, only: box, init_box
implicit none

type(box) :: x

call init_box(x)
if (abs(sum(x%values) - 3.0) > 1.0e-6) error stop
end program
