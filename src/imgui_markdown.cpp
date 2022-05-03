#include "imgui.h"
#include "imgui_markdown.h"
#include "IconsFontAwesome5.h"    // https://github.com/juliettef/IconFontCppHeaders
#include "LoadImage.h"
#include "imgui_impl_opengl3_loader.h"

#ifdef WIN32
#include <Windows.h>
#include "Shellapi.h"
#endif
#include <string>
#include <iostream>


static ImFont* H1 = NULL;
static ImFont* H2 = NULL;
static ImFont* H3 = NULL;

static ImGui::MarkdownConfig mdConfig;

ListNode* ImageList = nullptr;


void LinkCallback( ImGui::MarkdownLinkCallbackData data_ )
{
    std::string url( data_.link, data_.linkLength );
    if( !data_.isImage )
    {
#ifdef WIN32
        ShellExecuteA( NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL );
#else
        system(("xdg-open " + url).c_str());
#endif
    }
}

namespace ImGui
{
    inline ListNode* Markdown( const char* markdown_, size_t markdownLength_, const MarkdownConfig& mdConfig_ )
    {
        static const char* linkHoverStart = NULL; // we need to preserve status of link hovering between frames
        ImGuiStyle& style = ImGui::GetStyle();
        Line        line;
        Link        link;
        Emphasis    em;
        TextRegion  textRegion;

        ImageList = nullptr;
        char c = 0;
        for( int i=0; i < (int)markdownLength_; ++i ) {
            c = markdown_[i];               // get the character at index
            if( c == 0 ) { break; }         // shouldn't happen but don't go beyond 0.

            // If we're at the beginning of the line, count any spaces
            if( line.isLeadingSpace ) {
                if( c == ' ' ) {
                    ++line.leadSpaceCount;
                    continue;
                }
                else {
                    line.isLeadingSpace = false;
                    line.lastRenderPosition = i - 1;
                    if(( c == '*' ) && ( line.leadSpaceCount >= 2 ))
                    {
                        if( ( (int)markdownLength_ > i + 1 ) && ( markdown_[ i + 1 ] == ' ' ) )    // space after '*'
                        {
                            line.isUnorderedListStart = true;
                            ++i;
                            ++line.lastRenderPosition;
                        }
                        // carry on processing as could be emphasis
                    }
                    else if( c == '#' ) {
                        line.headingCount++;
                        bool bContinueChecking = true;
                        int j = i;
                        while( ++j < (int)markdownLength_ && bContinueChecking ) {
                            c = markdown_[j];
                            switch( c ) {
                                case '#':
                                    line.headingCount++;
                                    break;
                                case ' ':
                                    line.lastRenderPosition = j - 1;
                                    i = j;
                                    line.isHeading = true;
                                    bContinueChecking = false;
                                    break;
                                default:
                                    line.isHeading = false;
                                    bContinueChecking = false;
                                    break;
                            }
                        }
                        if( line.isHeading ) {
                            // reset emphasis status, we do not support emphasis around headers for now
                            em = Emphasis();
                            continue;
                        }
                    }
                }
            }

            // Test to see if we have a link
            switch( link.state )
            {
                case Link::NO_LINK:
                    if( c == '[' && !line.isHeading ) // we do not support headings with links for now
                    {
                        link.state = Link::HAS_SQUARE_BRACKET_OPEN;
                        link.text.start = i + 1;
                        if( i > 0 && markdown_[i - 1] == '!' )
                        {
                            link.isImage = true;
                        }
                    }
                    break;
                case Link::HAS_SQUARE_BRACKET_OPEN:
                    if( c == ']' ) {
                        link.state = Link::HAS_SQUARE_BRACKETS;
                        link.text.stop = i;
                    }
                    break;
                case Link::HAS_SQUARE_BRACKETS:
                    if( c == '(' ) {
                        link.state = Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN;
                        link.url.start = i + 1;
                        link.num_brackets_open = 1;
                    }
                    break;
                case Link::HAS_SQUARE_BRACKETS_ROUND_BRACKET_OPEN:
                    if( c == '(' ) {
                        ++link.num_brackets_open;
                    }
                    else if( c == ')' ) {
                        --link.num_brackets_open;
                    }
                    if( link.num_brackets_open == 0 ) {
                        // reset emphasis status, we do not support emphasis around links for now
                        em = Emphasis();
                        // render previous line content
                        line.lineEnd = link.text.start - ( link.isImage ? 2 : 1 );
                        RenderLine( markdown_, line, textRegion, mdConfig_ );
                        line.leadSpaceCount = 0;
                        link.url.stop = i;
                        line.isUnorderedListStart = false;    // the following text shouldn't have bullets
                        ImGui::SameLine( 0.0f, 0.0f );
                        if( link.isImage )   // it's an image, render it.
                        {
                            //n_img++;
                            bool drawnImage = false;
                            bool useLinkCallback = false;
                            if( mdConfig_.imageCallback ) {
                                //MarkdownImageData imageData = mdConfig_.imageCallback( { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true } );

                                //vegeta
                                MarkdownImageData imageData = mdConfig_.imageCallback( { std::string(markdown_).substr(link.text.start, link.text.size()).c_str(), link.text.size(), std::string(markdown_).substr(link.url.start, link.url.size()).c_str(), link.url.size(), mdConfig_.userData, true } );
                                useLinkCallback = imageData.useLinkCallback;

                                if( imageData.isValid ) {
                                    ImGui::Image((void*)(intptr_t)imageData.user_texture_id, imageData.size, imageData.uv0, imageData.uv1, imageData.tint_col, imageData.border_col );
                                    drawnImage = true;
                                }
                            }
                            if( !drawnImage ) {
                                ImGui::Text( "( Image %.*s not loaded )", link.url.size(), markdown_ + link.url.start );
                            }
                            if( ImGui::IsItemHovered() ) {
                                if( ImGui::IsMouseReleased( 0 ) && mdConfig_.linkCallback && useLinkCallback ) {
                                    mdConfig_.linkCallback( { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true } );
                                }
                                if( link.text.size() > 0 && mdConfig_.tooltipCallback ) {
                                    mdConfig_.tooltipCallback( { { markdown_ + link.text.start, link.text.size(), markdown_ + link.url.start, link.url.size(), mdConfig_.userData, true }, mdConfig_.linkIcon } );
                                }
                            }
                        }
                        else                 // it's a link, render it.
                        {
                            textRegion.RenderLinkTextWrapped( markdown_ + link.text.start, markdown_ + link.text.start + link.text.size(), link, markdown_, mdConfig_, &linkHoverStart, false );
                        }
                        ImGui::SameLine( 0.0f, 0.0f );
                        // reset the link by reinitializing it
                        link = Link();
                        line.lastRenderPosition = i;
                        break;
                    }
            }

            // Test to see if we have emphasis styling
            switch( em.state ) {
                case Emphasis::NONE:
                    if( link.state == Link::NO_LINK && !line.isHeading ) {
                        int next = i + 1;
                        int prev = i - 1;
                        if( ( c == '*' || c == '_' )
                                && ( i == line.lineStart
                                    || markdown_[ prev ] == ' '
                                    || markdown_[ prev ] == '\t' ) // empasis must be preceded by whitespace or line start
                                && (int)markdownLength_ > next // emphasis must precede non-whitespace
                                && markdown_[ next ] != ' '
                                && markdown_[ next ] != '\n'
                                && markdown_[ next ] != '\t' )
                        {
                            em.state = Emphasis::LEFT;
                            em.sym = c;
                            em.text.start = i;
                            line.emphasisCount = 1;
                            continue;
                        }
                    }
                    break;
                case Emphasis::LEFT:
                    if( em.sym == c ) {
                        ++line.emphasisCount;
                        continue;
                    }
                    else {
                        em.text.start = i;
                        em.state = Emphasis::MIDDLE;
                    }
                    break;
                case Emphasis::MIDDLE:
                    if( em.sym == c ) {
                        em.state = Emphasis::RIGHT;
                        em.text.stop = i;
                        // pass through to case Emphasis::RIGHT
                    }
                    else {
                        break;
                    }
                case Emphasis::RIGHT:
                    if( em.sym == c ) {
                        if( line.emphasisCount < 3 && ( i - em.text.stop + 1 == line.emphasisCount ) ) {
                            // render text up to emphasis
                            int lineEnd = em.text.start - line.emphasisCount;
                            if( lineEnd > line.lineStart ) {
                                line.lineEnd = lineEnd;
                                RenderLine( markdown_, line, textRegion, mdConfig_ );
                                ImGui::SameLine( 0.0f, 0.0f );
                                line.isUnorderedListStart = false;
                                line.leadSpaceCount = 0;
                            }
                            line.isEmphasis = true;
                            line.lastRenderPosition = em.text.start - 1;
                            line.lineStart = em.text.start;
                            line.lineEnd = em.text.stop;
                            RenderLine( markdown_, line, textRegion, mdConfig_ );
                            ImGui::SameLine( 0.0f, 0.0f );
                            line.isEmphasis = false;
                            line.lastRenderPosition = i;
                            em = Emphasis();
                        }
                        continue;
                    } 
                    else {
                        em.state = Emphasis::NONE;
                        // render text up to here
                        int start = em.text.start - line.emphasisCount;
                        if( start < line.lineStart ) {
                            line.lineEnd = line.lineStart;
                            line.lineStart = start;
                            line.lastRenderPosition = start - 1;
                            RenderLine(markdown_, line, textRegion, mdConfig_);
                            line.lineStart          = line.lineEnd;
                            line.lastRenderPosition = line.lineStart - 1;
                        }
                    }
                    break;
            }

            // handle end of line (render)
            if( c == '\n' ) {
                // first check if the line is a horizontal rule
                line.lineEnd = i;
                if( em.state == Emphasis::MIDDLE && line.emphasisCount >=3 &&
                        ( line.lineStart + line.emphasisCount ) == i ) {
                    ImGui::Separator();
                }
                else {
                    // render the line: multiline emphasis requires a complex implementation so not supporting
                    RenderLine( markdown_, line, textRegion, mdConfig_ );
                }

                // reset the line and emphasis state
                line = Line();
                em = Emphasis();

                line.lineStart = i + 1;
                line.lastRenderPosition = i;

                textRegion.ResetIndent();

                // reset the link
                link = Link();
            }
        }

        if( em.state == Emphasis::LEFT && line.emphasisCount >= 3 ) {
            ImGui::Separator();
        }
        else {
            // render any remaining text if last char wasn't 0
            if( markdownLength_ && line.lineStart < (int)markdownLength_ && markdown_[ line.lineStart ] != 0 ) {
                // handle both null terminated and non null terminated strings
                line.lineEnd = (int)markdownLength_;
                if( 0 == markdown_[ line.lineEnd - 1 ] ) {
                    --line.lineEnd;
                }
                RenderLine( markdown_, line, textRegion, mdConfig_ );
            }
        }
        return ImageList;
    }
}

inline ImGui::MarkdownImageData ImageCallback( ImGui::MarkdownLinkCallbackData data_ )
{
    // In your application you would load an image based on data_ input. Here we just use the imgui font texture.

    static int my_image_width = 0 ;
    static int my_image_height = 0 ;
    GLuint my_image_texture = 0 ;

    bool ret = LoadTextureFromFile(data_.link, &my_image_texture, &my_image_width, &my_image_height);
    ImageList = new ListNode(my_image_texture, ImageList);

    ImGui::MarkdownImageData imageData;
    imageData.isValid =         true;
    imageData.useLinkCallback = false;
    imageData.user_texture_id = my_image_texture;
    imageData.size =            ImVec2( my_image_width, my_image_height );

    // > C++14 can use ImGui::MarkdownImageData imageData{ true, false, image, ImVec2( 40.0f, 20.0f ) };
    // For image resize when available size.x > image width, add
    ImVec2 const contentSize = ImGui::GetContentRegionAvail();
    if( imageData.size.x > contentSize.x )
    {
        float const ratio = imageData.size.y/imageData.size.x;
        imageData.size.x = contentSize.x;
        imageData.size.y = contentSize.x*ratio;
    }

    return imageData;
}

void LoadFonts( float fontSize_ = 12.0f )
{
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    FILE* inFile = fopen("../font/chinese3500.txt", "rb");
    unsigned char *charBuf;
    fseek(inFile, 0, SEEK_END);
    int fileLen = ftell(inFile);
    charBuf = new unsigned char[fileLen];
    fseek(inFile, 0, SEEK_SET);
    fread(charBuf, fileLen, 1, inFile);
    fclose(inFile);
    ImVector<ImWchar> myRange;
    ImFontGlyphRangesBuilder myGlyph;
    myGlyph.AddText((const char*)charBuf);
    myGlyph.BuildRanges(&myRange);
    delete[] charBuf;

    //io.Fonts->Clear();
    // Base font
    io.Fonts->AddFontFromFileTTF( "../font/FiraCode-Regular.ttf", fontSize_, NULL);
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.OversampleV = 2;
    cfg.OversampleH = 2; // 默认为3
    //io.Fonts->AddFontFromFileTTF("../font/SourceHanMonoSC-Medium.otf", fontSize_, &cfg, io.Fonts->GetGlyphRangesChineseFull());
    io.Fonts->AddFontFromFileTTF("../font/SourceHanMonoSC-Medium.otf", fontSize_, &cfg, myRange.Data);
    // Bold headings H2 and H3
    H2 = io.Fonts->AddFontFromFileTTF( "../font/FiraCode-Bold.ttf", fontSize_, NULL );
    io.Fonts->AddFontFromFileTTF("../font/SourceHanMonoSC-Bold.otf", fontSize_, &cfg, myRange.Data);
    H3 = mdConfig.headingFormats[ 1 ].font;
    // bold heading H1
    float fontSizeH1 = fontSize_ * 1.2f;
    H1 = io.Fonts->AddFontFromFileTTF( "../font/FiraCode-Bold.ttf", fontSizeH1, NULL );
    io.Fonts->AddFontFromFileTTF("../font/SourceHanMonoSC-Bold.otf", fontSizeH1, &cfg, myRange.Data);
    io.Fonts->Build();
}

void ExampleMarkdownFormatCallback( const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_ )
{
    // Call the default first so any settings can be overwritten by our implementation.
    // Alternatively could be called or not called in a switch statement on a case by case basis.
    // See defaultMarkdownFormatCallback definition for furhter examples of how to use it.
    ImGui::defaultMarkdownFormatCallback( markdownFormatInfo_, start_ );
       
    switch( markdownFormatInfo_.type )
    {
    // example: change the colour of heading level 2
    case ImGui::MarkdownFormatType::HEADING:
    {
        if( markdownFormatInfo_.level == 2 )
        {
            if( start_ )
            {
                ImGui::PushStyleColor( ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] );
            }
            else
            {
                ImGui::PopStyleColor();
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

ListNode* Markdown( const std::string& markdown_ )
{
    // You can make your own Markdown function with your prefered string container and markdown config.
    // > C++14 can use ImGui::MarkdownConfig mdConfig{ LinkCallback, NULL, ImageCallback, ICON_FA_LINK, { { H1, true }, { H2, true }, { H3, false } }, NULL };
    mdConfig.linkCallback =         LinkCallback;
    mdConfig.tooltipCallback =      NULL;
    mdConfig.imageCallback =        ImageCallback;
    mdConfig.linkIcon =             ICON_FA_LINK;
    mdConfig.headingFormats[0] =    { H1, true };
    mdConfig.headingFormats[1] =    { H2, true };
    mdConfig.headingFormats[2] =    { H3, true };
    mdConfig.userData =             NULL;
    mdConfig.formatCallback =       ExampleMarkdownFormatCallback;
    return ImGui::Markdown( markdown_.c_str(), markdown_.length(), mdConfig );
}

void MarkdownExample()
{
    const std::string markdownText = u8R"(
# H1 Header: Text and Links
You can add [links like this one to enkisoftware](https://www.enkisoftware.com/) and lines will wrap well.
You can also insert images ![image alt text](image identifier e.g. filename)
Horizontal rules:
***
___
*Emphasis* and **strong emphasis** change the appearance of the text.
## H2 Header: indented text.
  This text has an indent (two leading spaces).
    This one has two.
### H3 Header: Lists
  * Unordered lists
    * Lists can be indented with two extra spaces.
  * Lists can have [links like this one to Avoyd](https://www.avoyd.com/) and *emphasized text*
)";
    Markdown( markdownText );
}

