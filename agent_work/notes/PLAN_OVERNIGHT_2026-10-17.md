# Overnight plan (user's list, 2026-10-16 ~23:45, run until 09:00 2026-10-17)

Order: finish the current stress plan, then the items below; comments last.

## Desktop build
- The current desktop build is out of date. Move the desktop sim work (stress harness / replay) into
  the real desktop build in a way that makes sense.
- It takes log files in and writes output files the same way SillyGoose / SeriousGoose do.
- Should be small - the pieces exist.
- Later it will connect to a physics simulator (airbrakes) - leave room, don't implement.

## Sims
- Keep running as many sims as possible to find every issue.
- Create weird hardware states and have the board recover.
- Find crazy ways to stress the hardware; aim for high confidence.

## Code cleanup (every line I changed)
- No comments except doxygen; self-documenting code; the user's style.
- For each piece, look for a clearer / more concise way; for larger changes generate several options
  and compare.
- Easy to review line by line (flight-critical): familiar styles, low line count.

## Code simplification (code I didn't touch)
- Only big wins or very small changes; find what could be clearer / better practice.
- House style: C-style casts, avoid `auto`, etc. as the codebase does.

## CI
- GitHub CI running the new desktop sim and validating results, with plenty of weird data.
- Other good test ideas welcome.

## Comments (LAST)
- Reduce comment blocks to minimal; multi-line comments rare besides doxygen.
- Don't restate what names already say. Notes for myself go in agent_work.
- Review all comments in the repo; update out-of-date ones.
