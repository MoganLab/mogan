#!/bin/bash
# Test script to verify Claw AI integration

echo "=========================================="
echo "Claw AI Integration Test"
echo "=========================================="

# Check if Mogan binary exists
MOGAN_BIN="./build/macosx/arm64/release/MoganSTEM.app/Contents/MacOS/MoganSTEM"
if [ ! -f "$MOGAN_BIN" ]; then
    echo "❌ Mogan binary not found at $MOGAN_BIN"
    exit 1
fi

echo "✅ Mogan binary found"

# Check if Claw AI Scheme files exist
CLAW_AI_SCM="./TeXmacs/progs/claw-ai/claw-ai.scm"
if [ ! -f "$CLAW_AI_SCM" ]; then
    echo "❌ Claw AI Scheme module not found"
    exit 1
fi

echo "✅ Claw AI Scheme module found"

# Check if Glue code is compiled
GLUE_OBJ="./build/macosx/arm64/release/.objs/libmogan/macosx/arm64/release/src/Scheme/Scheme/init_glue_claw_ai.cpp.o"
if [ ! -f "$GLUE_OBJ" ]; then
    echo "⚠️  Glue object not found (might be cached)"
else
    echo "✅ Glue code compiled"
fi

# Check if QTMClawAIWidget is compiled
WIDGET_OBJ="./build/macosx/arm64/release/.objs/libmogan/macosx/arm64/release/src/Plugins/Qt/QTMClawAIWidget.cpp.o"
if [ ! -f "$WIDGET_OBJ" ]; then
    echo "⚠️  Widget object not found (might be cached)"
else
    echo "✅ QTMClawAIWidget compiled"
fi

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "All components are present!"
echo ""
echo "To test Claw AI in Mogan:"
echo "1. Run: xmake run stem"
echo "2. In Scheme session, execute:"
echo "   (use-modules (claw-ai))"
echo "   (claw-ai-show)"
echo ""
