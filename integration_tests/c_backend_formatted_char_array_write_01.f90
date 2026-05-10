program c_backend_formatted_char_array_write_01
    implicit none
    character(1) :: letters(4)
    character(8) :: line

    letters = ["a", "b", "c", "d"]
    line = ""
    write(line, '(4a)') letters

    if (line(1:4) /= "abcd") error stop
end program
