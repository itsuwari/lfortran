program c_backend_block_return_slot_cleanup_ref_01
implicit none

character(:), allocatable :: base

block
    call take(make_string())
end block

base = "associate-temp"
associate (view => base)
    call take_associate(view // make_suffix())
end associate
deallocate(base)

contains

    function make_string() result(out)
        character(:), allocatable :: out
        out = "block-temp"
    end function

    function make_suffix() result(out)
        character(:), allocatable :: out
        out = ""
    end function

    subroutine take(value)
        character(*), intent(in) :: value
        if (value /= "block-temp") error stop
    end subroutine

    subroutine take_associate(value)
        character(*), intent(in) :: value
        if (value /= "associate-temp") error stop
    end subroutine

end program
