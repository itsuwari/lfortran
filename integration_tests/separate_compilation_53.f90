program separate_compilation_53
use separate_compilation_53a, only: child_t, dispatch_value
implicit none

type(child_t) :: item

if (dispatch_value(item) /= 53) error stop

end program
