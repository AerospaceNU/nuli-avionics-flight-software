#!/usr/bin/env bash


buildCoverageReport() {
  echo -e "\e[1;35m Generating test coverage report... \e[0m"

  animate &
  local pid=$!

  {
    cd ../../../../   # assuming in platformio_nuli_avionics_flight_software > include > cli > tests (yes this is shit implementation)

    rm -rf build

    cmake -S . -B build -DENABLE_COVERAGE=ON

    cmake --build build

    cd build || (echo e "\e[1;31m [BUILD FAILED] Unable to create 'build' directory \e[0m" ; exit)

    ctest --verbose

    lcov --capture --directory . --output-file coverage.info

    lcov --remove coverage.info '/usr/*' '*/tests/*' '*/gtest/*' --output-file coverage_filtered.info

    genhtml coverage_filtered.info --output-directory coverage_report --demangle-cpp
  } > /dev/null 2>&1

  kill $pid
  printf "\r[Success] Done!         \n"
}

# did I spend too much time writing this and testing how much the animation slows the testing down? yes.
animate() {
    local frames=("|" "/" "-" "\\")            # Spinner frames
    local dotFrames=(".     " "..    " "...   " "....  " "..... " "......") # Dots

    local frameIndex=0
    local dotIndex=0
    local frameCount=${#frames[@]}
    local dotCount=${#dotFrames[@]}

    local elapsedTime=0

    while true ; do  # kill -0 will return true while $pid is running
        printf "\r[%s] Processing%s" "${frames[$frameIndex]}" "${dotFrames[$dotIndex]}"

        if [ $(( elapsedTime % 3 )) == 0 ]; then  # update spinner every 0.3s
          frameIndex=$(( (frameIndex + 1) % frameCount ))
        fi

        if [ $(( elapsedTime % 2 )) == 0 ]; then  # update dots every 0.2s
          dotIndex=$(( (dotIndex + 1) % dotCount ))
        fi

        sleep 0.1  # Adjust sleep to manage speed
        elapsedTime=$((elapsedTime + 1))
    done
}

buildCoverageReport
