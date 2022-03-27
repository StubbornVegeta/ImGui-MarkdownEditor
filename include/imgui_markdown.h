#pragma once

// License: zlib
// Copyright (c) 2019 Juliette Foucaut & Doug Binks
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

/*
API BREAKING CHANGES
====================
- 2020/04/22 - Added tooltipCallback parameter to ImGui::MarkdownConfig
- 2019/02/01 - Changed LinkCallback parameters, see https://github.com/juliettef/imgui_markdown/issues/2
- 2019/02/05 - Added imageCallback parameter to ImGui::MarkdownConfig
- 2019/02/06 - Added useLinkCallback member variable to MarkdownImageData to configure using images as links
*/

#include <stdint.h>
#include <iostream>
#include <string>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui.h"
#include "LoadImage.h"
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

struct ListNode {
    GLuint value;
    ListNode* next;
    ListNode() : value(0), next(nullptr) {}
    ListNode(GLuint val_, ListNode* next_)
        : value(val_), next(next_) {}
};

namespace ImGui {
    //-----------------------------------------------------------------------------
    // Basic types
    //-----------------------------------------------------------------------------

    struct Link;
    struct MarkdownConfig;

    struct MarkdownLinkCallbackData                                 // for both links and images
    {
        const char*             text;                               // text between square brackets []
        int                     textLength;
        const char*             link;                               // text between brackets ()
        int                     linkLength;
        void*                   userData;
        bool                    isImage;                            // true if '!' is detected in front of the link syntax
        //int                     ImageNumber = 0;
    };

    struct MarkdownTooltipCallbackData                              // for tooltips
    {
        MarkdownLinkCallbackData linkData;
        const char*              linkIcon;
    };
    
    struct MarkdownImageData {
        bool                    isValid = false;                    // if true, will draw the image
        bool                    useLinkCallback = false;            // if true, linkCallback will be called when image is clicked
        //ImTextureID             user_texture_id = 0;                // see ImGui::Image
        GLuint           user_texture_id = 0;                // see ImGui::Image

        ImVec2                  size = ImVec2( 100.0f, 100.0f );    // see ImGui::Image
        ImVec2                  uv0 = ImVec2( 0, 0 );               // see ImGui::Image
        ImVec2                  uv1 = ImVec2( 1, 1 );               // see ImGui::Image
        ImVec4                  tint_col = ImVec4( 1, 1, 1, 1 );    // see ImGui::Image
        ImVec4                  border_col = ImVec4( 0, 0, 0, 0 );  // see ImGui::Image
    };

    enum class MarkdownFormatType {
         NORMAL_TEXT,
         HEADING,
         UNORDERED_LIST,
         LINK,
         EMPHASIS,
    };

    struct MarkdownFormatInfo {
        MarkdownFormatType      type    = MarkdownFormatType::NORMAL_TEXT;
        int32_t                 level   = 0;                               // Set for headings: 1 for H1, 2 for H2 etc.
        bool                    itemHovered = false;                       // Currently only set for links when mouse hovered, only valid when start_ == false
        const MarkdownConfig*   config  = NULL;
    };

    typedef void                MarkdownLinkCallback( MarkdownLinkCallbackData data );    
    typedef void                MarkdownTooltipCallback( MarkdownTooltipCallbackData data );

    inline void defaultMarkdownTooltipCallback( MarkdownTooltipCallbackData data_ ) {
        if( data_.linkData.isImage ) {
            ImGui::SetTooltip( "%.*s", data_.linkData.linkLength, data_.linkData.link );
        }
        else {
            ImGui::SetTooltip( "%s Open in browser\n%.*s", data_.linkIcon, data_.linkData.linkLength, data_.linkData.link );
        }
    }

    //typedef MarkdownImageData   MarkdownImageCallback( MarkdownLinkCallbackData data );
    typedef MarkdownImageData   ImageCallback( MarkdownLinkCallbackData data );
    typedef void                MarkdownFormalCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

    inline void defaultMarkdownFormatCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

    struct MarkdownHeadingFormat {
        ImFont*                 font;                               // ImGui font
        bool                    separator;                          // if true, an underlined separator is drawn after the header
    };

    // Configuration struct for Markdown
    // - linkCallback is called when a link is clicked on
    // - linkIcon is a string which encode a "Link" icon, if available in the current font (e.g. linkIcon = ICON_FA_LINK with FontAwesome + IconFontCppHeaders https://github.com/juliettef/IconFontCppHeaders)
    // - headingFormats controls the format of heading H1 to H3, those above H3 use H3 format
    struct MarkdownConfig {
        static const int        NUMHEADINGS = 3;

        MarkdownLinkCallback*   linkCallback = NULL;
        MarkdownTooltipCallback* tooltipCallback = NULL;
        //MarkdownImageCallback*  imageCallback = NULL;
        ImageCallback*  imageCallback = NULL;
        const char*             linkIcon = "";                      // icon displayd in link tooltip
        MarkdownHeadingFormat   headingFormats[ NUMHEADINGS ] = { { NULL, true }, { NULL, true }, { NULL, true } };
        void*                   userData = NULL;
        MarkdownFormalCallback* formatCallback = defaultMarkdownFormatCallback;
    };

    //-----------------------------------------------------------------------------
    // External interface
    //-----------------------------------------------------------------------------

    inline ListNode* Markdown( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ );

    //-----------------------------------------------------------------------------
    // Internals
    //-----------------------------------------------------------------------------

    struct TextRegion;
    struct Line;
    inline void UnderLine( ImColor col_ );
    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ );

    struct TextRegion {
        TextRegion() : indentX( 0.0f )
        {
        }
        ~TextRegion()
        {
            ResetIndent();
        }

        // ImGui::TextWrapped will wrap at the starting position
        // so to work around this we render using our own wrapping for the first line
        void RenderTextWrapped( const char* text_, const char* text_end_, bool bIndentToHere_ = false ) {
            float       scale = ImGui::GetIO().FontGlobalScale;
            float       widthLeft = GetContentRegionAvail().x;
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
            ImGui::TextUnformatted( text_, endLine );
            if( bIndentToHere_ ) {
                float indentNeeded = GetContentRegionAvail().x - widthLeft;
                if( indentNeeded ) {
                    ImGui::Indent( indentNeeded );
                    indentX += indentNeeded;
                }
            }
            widthLeft = GetContentRegionAvail().x;
            while( endLine < text_end_ ) {
                text_ = endLine;
                if( *text_ == ' ' ) { ++text_; }    // skip a space at start of line
                endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                if( text_ == endLine ) {
                    endLine++;
                }
                ImGui::TextUnformatted( text_, endLine );
            }
        }

        void RenderListTextWrapped( const char* text_, const char* text_end_ ) {
            ImGui::Bullet();
            ImGui::SameLine();
            RenderTextWrapped( text_, text_end_, true );
        }

        bool RenderLinkText( const char* text_, const char* text_end_, const Link& link_, 
            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_ );

        void RenderLinkTextWrapped( const char* text_, const char* text_end_, const Link& link_,
            const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_, bool bIndentToHere_ = false );

        void ResetIndent() {
            if( indentX > 0.0f )
            {
                ImGui::Unindent( indentX );
            }
            indentX = 0.0f;
        }

    private:
        float       indentX;
    };

    // Text that starts after a new line (or at beginning) and ends with a newline (or at end)
    struct Line {
        bool isHeading = false;
        bool isEmphasis = false;
        bool isUnorderedListStart = false;
        bool isLeadingSpace = true;     // spaces at start of line
        int  leadSpaceCount = 0;
        int  headingCount = 0;
        int  emphasisCount = 0;
        int  lineStart = 0;
        int  lineEnd   = 0;
        int  lastRenderPosition = 0;     // lines may get rendered in multiple pieces
    };

    struct TextBlock {                  // subset of line
        int start = 0;
        int stop  = 0;
        int size() const {
            return stop - start;
        }
    };

    struct Link {
        enum LinkState {
            NO_LINK,
            HAS_SQUARE_BRACKET_OPEN,
            HAS_SQUARE_BRACKETS,
            HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN,
        };
        LinkState state = NO_LINK;
        TextBlock text;
        TextBlock url;
        bool isImage = false;
        int num_brackets_open = 0;
    };

    struct Emphasis {
        enum EmphasisState {
            NONE,
            LEFT,
            MIDDLE,
            RIGHT,
        };
        EmphasisState state = NONE;
        TextBlock text;
        char sym;
    };

    inline void UnderLine( ImColor col_ ) {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine( min, max, col_, 1.0f );
    }

    inline void RenderLine( const char* markdown_, Line& line_, TextRegion& textRegion_, const MarkdownConfig& mdConfig_ ) {
        // indent
        int indentStart = 0;
        if( line_.isUnorderedListStart )    // ImGui unordered list render always adds one indent
        { 
            indentStart = 1; 
        }
        for( int j = indentStart; j < line_.leadSpaceCount / 2; ++j )    // add indents
        {
            ImGui::Indent();
        }

        // render
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        int textStart = line_.lastRenderPosition + 1;
        int textSize = line_.lineEnd - textStart;
        if( line_.isUnorderedListStart )    // render unordered list
        {
            formatInfo.type = MarkdownFormatType::UNORDERED_LIST;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            textRegion_.RenderListTextWrapped( text, text + textSize - 1 );
        }
        else if( line_.isHeading )          // render heading
        {
            formatInfo.level = line_.headingCount;
            formatInfo.type = MarkdownFormatType::HEADING;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart + 1;
            textRegion_.RenderTextWrapped( text, text + textSize - 1 );
        }
        else if( line_.isEmphasis )         // render emphasis
        {
            formatInfo.level = line_.emphasisCount;
            formatInfo.type = MarkdownFormatType::EMPHASIS;
            mdConfig_.formatCallback(formatInfo, true);
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped(text, text + textSize);
        }
        else                                // render a normal paragraph chunk
        {
            formatInfo.type = MarkdownFormatType::NORMAL_TEXT;
            mdConfig_.formatCallback( formatInfo, true );
            const char* text = markdown_ + textStart;
            textRegion_.RenderTextWrapped( text, text + textSize );
        }
        mdConfig_.formatCallback( formatInfo, false );

        // unindent
        for( int j = indentStart; j < line_.leadSpaceCount / 2; ++j ) {
            ImGui::Unindent();
        }
    }
    
    // render markdown
    inline ListNode* Markdown( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ ); 

    inline bool TextRegion::RenderLinkText( const char* text_, const char* text_end_, const Link& link_,
        const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_ ) {
        MarkdownFormatInfo formatInfo;
        formatInfo.config = &mdConfig_;
        formatInfo.type = MarkdownFormatType::LINK;
        mdConfig_.formatCallback( formatInfo, true );
        ImGui::PushTextWrapPos( -1.0f );
        ImGui::TextUnformatted( text_, text_end_ );
        ImGui::PopTextWrapPos();

        bool bThisItemHovered = ImGui::IsItemHovered();
        if(bThisItemHovered) {
            *linkHoverStart_ = markdown_ + link_.text.start;
        }
        bool bHovered = bThisItemHovered || ( *linkHoverStart_ == ( markdown_ + link_.text.start ) );

        formatInfo.itemHovered = bHovered;
        mdConfig_.formatCallback( formatInfo, false );

        if(bHovered) {
            if( ImGui::IsMouseReleased( 0 ) && mdConfig_.linkCallback ) {
                mdConfig_.linkCallback( { markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false } );
            }
            if( mdConfig_.tooltipCallback ) {
                mdConfig_.tooltipCallback( { { markdown_ + link_.text.start, link_.text.size(), markdown_ + link_.url.start, link_.url.size(), mdConfig_.userData, false }, mdConfig_.linkIcon } );
            }
        }
        return bThisItemHovered;
    }

    inline void TextRegion::RenderLinkTextWrapped( const char* text_, const char* text_end_, const Link& link_,
        const char* markdown_, const MarkdownConfig& mdConfig_, const char** linkHoverStart_, bool bIndentToHere_ ) {
            float       scale = ImGui::GetIO().FontGlobalScale;
            float       widthLeft = GetContentRegionAvail().x;
            const char* endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
            bool bHovered = RenderLinkText( text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_ );
            if( bIndentToHere_ ) {
                float indentNeeded = GetContentRegionAvail().x - widthLeft;
                if( indentNeeded ) {
                    ImGui::Indent( indentNeeded );
                    indentX += indentNeeded;
                }
            }
            widthLeft = GetContentRegionAvail().x;
            while( endLine < text_end_ ) {
                text_ = endLine;
                if( *text_ == ' ' ) { ++text_; }    // skip a space at start of line
                endLine = ImGui::GetFont()->CalcWordWrapPositionA( scale, text_, text_end_, widthLeft );
                if( text_ == endLine ) {
                    endLine++;
                }
                bool bThisLineHovered = RenderLinkText( text_, endLine, link_, markdown_, mdConfig_, linkHoverStart_ );
                bHovered = bHovered || bThisLineHovered;
            }
            if( !bHovered && *linkHoverStart_ == markdown_ + link_.text.start ) {
                *linkHoverStart_ = NULL;
            }
        }


    inline void defaultMarkdownFormatCallback( const MarkdownFormatInfo& markdownFormatInfo_, bool start_ ) {
        switch( markdownFormatInfo_.type ) {
        case MarkdownFormatType::NORMAL_TEXT:
            break;
        case MarkdownFormatType::EMPHASIS: {
            MarkdownHeadingFormat fmt;
            // default styling for emphasis uses last headingFormats - for your own styling
            // implement EMPHASIS in your formatCallback
            if( markdownFormatInfo_.level == 1 )
            {
                // normal emphasis
                if( start_ ) {
                    ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ ImGuiCol_TextDisabled ] );
                }
                else {
                    ImGui::PopStyleColor();
                }
            }
            else {
                // strong emphasis
                fmt = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ];
                if( start_ ) {
                    if( fmt.font ) {
                        ImGui::PushFont( fmt.font );
                    }
                }
                else {
                    if( fmt.font ) {
                        ImGui::PopFont();
                    }
                }
            }
            break;
        }
        case MarkdownFormatType::HEADING:
        {
            MarkdownHeadingFormat fmt;
            if( markdownFormatInfo_.level > MarkdownConfig::NUMHEADINGS ) {
                fmt = markdownFormatInfo_.config->headingFormats[ MarkdownConfig::NUMHEADINGS - 1 ];
            }
            else {
                fmt = markdownFormatInfo_.config->headingFormats[ markdownFormatInfo_.level - 1 ];
            }
            if( start_ ) {
                if( fmt.font  ) {
                    ImGui::PushFont( fmt.font );
                }
                ImGui::NewLine();
            }
            else {
                if( fmt.separator ) {
                    ImGui::Separator();
                    ImGui::NewLine();
                }
                else {
                    ImGui::NewLine();
                }
                if( fmt.font ) {
                    ImGui::PopFont();
                }
            }
            break;
        }
        case MarkdownFormatType::UNORDERED_LIST:
            break;
        case MarkdownFormatType::LINK:
            if( start_ ) {
                ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ ImGuiCol_ButtonHovered ] );
            }
            else {
                ImGui::PopStyleColor();
                if( markdownFormatInfo_.itemHovered ) {
                    ImGui::UnderLine( ImGui::GetStyle().Colors[ ImGuiCol_ButtonHovered ] );
                }
                else {
                    ImGui::UnderLine( ImGui::GetStyle().Colors[ ImGuiCol_Button ] );
                }
            }
            break;
        }
    }
}


void LinkCallback( ImGui::MarkdownLinkCallbackData data_ );

inline ImGui::MarkdownImageData ImageCallback( ImGui::MarkdownLinkCallbackData data_ );

void LoadFonts( float fontSize_ );

void ExampleMarkdownFormatCallback( const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_ );

ListNode* Markdown( const std::string& markdown_ );

void MarkdownExample();
