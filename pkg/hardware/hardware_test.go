// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package hardware

import (
	"testing"
)

func TestDetectHardware(t *testing.T) {
	info := Detect()

	if info.Model == "" {
		t.Error("Hardware model should not be empty")
	}

	if info.Cores < 1 {
		t.Error("CPU cores should be at least 1")
	}
}

func TestSimulatedClaw(t *testing.T) {
	claw := NewSimulatedClaw()

	// Test Open
	err := claw.Open()
	if err != nil {
		t.Errorf("Open failed: %v", err)
	}

	// Test Close
	err = claw.Close()
	if err != nil {
		t.Errorf("Close failed: %v", err)
	}

	// Test SetGripperAngle
	err = claw.SetGripperAngle(90)
	if err != nil {
		t.Errorf("SetGripperAngle failed: %v", err)
	}

	// Test angle validation
	err = claw.SetGripperAngle(200)
	if err == nil {
		t.Error("Expected error for angle > 180")
	}

	// Test Status
	status := claw.Status()
	if status == "" {
		t.Error("Status should not be empty")
	}
}

func TestAngleRange(t *testing.T) {
	claw := NewSimulatedClaw()

	testCases := []struct {
		angle    int
		shouldOk bool
	}{
		{0, true},
		{90, true},
		{180, true},
		{-1, false},
		{181, false},
	}

	for _, tc := range testCases {
		err := claw.SetGripperAngle(tc.angle)
		if tc.shouldOk && err != nil {
			t.Errorf("Angle %d should be valid, got error: %v", tc.angle, err)
		}
		if !tc.shouldOk && err == nil {
			t.Errorf("Angle %d should be invalid", tc.angle)
		}
	}
}
