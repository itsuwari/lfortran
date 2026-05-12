program c_backend_transfer_char_array_to_scalar_01
implicit none

character(len=1) :: bytes(4)
character(len=4) :: word
character(len=2) :: short_word

bytes = [character(len=1) :: "W", "x", "Y", "z"]

word = transfer(bytes, word)
if (word /= "WxYz") error stop "wrong full fixed character transfer"

short_word = transfer(bytes, short_word)
if (short_word /= "Wx") error stop "wrong short fixed character transfer"

end program c_backend_transfer_char_array_to_scalar_01
