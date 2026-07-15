-------------------------------------------------------------------------------
--
-- MODULE      : options.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

-- Add options for different features
option("style_agent")
    set_default(false)
    set_description("Enable Style Agent")
option_end()

option("windows_system_borders")
    set_default(false)
    set_description("Enable Windows System Borders")
option_end()

option("qt_frontend")
    set_default(true)
    set_description("Build the Qt frontend (default: ImGui frontend)")
option_end()

option("pdfhummus")
    set_default(true)
    set_description("Enable PDFHummus plugin")
option_end()

option("goldfish")
    set_default(true)
    set_description("Enable Goldfish plugin")
option_end()

option("mupdf")
    set_default(true)
    set_description("Enable MuPDF library")
option_end()

-- Temporary statement to move into MuPDF
set_config("mupdf", true)

option("startup_tab")
    set_default(true)
    set_description("Enable startup tab with left navigation")
option_end()

option("text_toolbar")
    set_default(false)
    set_description("Enable text selection floating toolbar")
option_end()

-- Adjust community or commercial version
option("is_community")
    set_default(is_community)
    set_description("Adjust community or commercial version")
option_end()

option("debug_with_timestamp")
    set_default(true)
    set_description("Enable timestamps in debug messages")
option_end()