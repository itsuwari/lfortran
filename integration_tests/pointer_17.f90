program pointer_17
  implicit none

  type :: item
    integer :: value = 0
  end type

  type(item), pointer :: p

  call clear(p)
  if (associated(p)) error stop "caller pointer should be disassociated"

contains

  subroutine clear(ptr)
    type(item), pointer, intent(out) :: ptr

    nullify(ptr)
    if (associated(ptr)) error stop "dummy pointer should be disassociated"
  end subroutine

end program
