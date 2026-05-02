program implied_do_loops38
  implicit none

  type :: line_token
    integer :: first, last
  end type line_token

  type(line_token) :: token(3), label(2)
  integer :: line(2), it

  token = [line_token(1, 0), line_token(3, 0), line_token(5, 0)]
  label = [line_token(2, 0), line_token(4, 0)]

  line(:) = [(count(token%first <= label(it)%first), it = 1, size(label))]

  if (line(1) /= 1) error stop "line(1)"
  if (line(2) /= 2) error stop "line(2)"
end program implied_do_loops38
