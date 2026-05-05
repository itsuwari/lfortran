program c_backend_allocatable_string_intent_out_cleanup_ref_01
implicit none

call check_lines()

contains

subroutine check_lines()
    character(:), allocatable :: line

    call next_line(line, "first")
    if (line /= "first") error stop

    call next_line(line, "second")
    if (line /= "second") error stop
end subroutine

subroutine next_line(line, value)
    character(:), allocatable, intent(out) :: line
    character(*), intent(in) :: value

    line = value
end subroutine

end program
