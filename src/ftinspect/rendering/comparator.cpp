// glyphbitmap.cpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#include "comparator.hpp"

#include <cmath>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtDebug>

  /* baseline distance between header lines */
#define HEADER_HEIGHT  12

static const char*  default_text =
    "Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Cras sit amet"
    " dui.  Nam sapien. Fusce vestibulum ornare metus. Maecenas ligula orci,"
    " consequat vitae, dictum nec, lacinia non, elit. Aliquam iaculis"
    " molestie neque. Maecenas suscipit felis ut pede convallis malesuada."
    " Aliquam erat volutpat. Nunc pulvinar condimentum nunc. Donec ac sem vel"
    " leo bibendum aliquam. Pellentesque habitant morbi tristique senectus et"
    " netus et malesuada fames ac turpis egestas.";


Comparator::Comparator(FT_Library lib,
                        FT_Face face,
                        FT_Size  size,
                        QStringList fontList,
                        int load_flags[],
                        int pixelMode[],
                        QVector<QRgb> grayColorTable,
                        QVector<QRgb> monoColorTable,
                        bool warping[],
                        bool kerningCol[],
                        double gammaVal,
                        unsigned long loadFlags)
: library(lib),
face(face),
size(size),
fontList(fontList),
grayColorTable(grayColorTable),
monoColorTable(monoColorTable),
gamma(gammaVal),
loadFlags(loadFlags)
{
  load[0] = load_flags[0];
  load[1] = load_flags[1];
  load[2] = load_flags[2];

  warping_col[0] = warping[0];
  warping_col[1] = warping[1];
  warping_col[2] = warping[2];

  pixelMode_col[0] = pixelMode[0];
  pixelMode_col[1] = pixelMode[1];
  pixelMode_col[2] = pixelMode[2];

  kerning[0] = kerningCol[0];
  kerning[1] = kerningCol[1];
  kerning[2] = kerningCol[2];
}


Comparator::~Comparator()
{
}

QRectF
Comparator::boundingRect() const
{
  return QRectF(-350, -250,
              700, 500);
}


void
Comparator::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget*)
{
  const char*  p;
  const char*  pEnd;
  int          ch;
  FT_UInt  glyph_idx;
  long load_flags[3];
  int column_selected;
  
  p    = default_text;
  pEnd = p + strlen( default_text ); 
  int length = strlen(default_text);

  int  border_width;

  int  column_x_start[3];
  int  column_x_end[3];
  int  column_x_temp[3];
  int  column_y_start;

  int  column_height;
  int  column_width;
  
  /* We have this layout:                                */
  /*                                                     */
  /*  | n ----x---- n  n ----x---- n  n ----x---- n |    */
  /*                                                     */
  /* w = 6 * n + 3 * x                                   */

  border_width = 10;                                /* n */
  int width = -350;
  column_width = ( 700 - 6 * border_width ) / 3;  /* x */

  column_x_start[0] = width + border_width;
  column_x_start[1] = width + 3 * border_width + column_width;
  column_x_start[2] = width + 5 * border_width + 2 * column_width;

  column_x_end[0] = column_x_start[0] + column_width;
  column_x_end[1] = column_x_start[1] + column_width;
  column_x_end[2] = column_x_start[2] + column_width;

  column_x_temp[0] = width + border_width;
  column_x_temp[1] = width + 3 * border_width + column_width;
  column_x_temp[2] = width + 5 * border_width + 2 * column_width;

  const qreal lod = option->levelOfDetailFromTransform(
                            painter->worldTransform());

  int height = -220;

  /* error = FT_New_Face(library,
                fontList[0].toLatin1().constData(),
                0,
                &f);*/

  error = FT_Set_Char_Size(face,
                        0,
                        16 * 64,
                        0,
                        72);
                        
  //column_y_start = 10 + 2 * HEADER_HEIGHT;
  //column_height  = height - 8 * HEADER_HEIGHT - 5;
  for (int col = 0; col < 3; col++)
  {
    FT_UInt previous;
    height = -230;

    FT_Error error = FT_Property_Set(library,
                              "autofitter",
                              "warping",
                              &warping_col[col]);

    int count = 0;

    for ( int i = 0; i < length; i++ )
    {

      count += 1;
      QChar ch = default_text[i];

      // get char index
      glyph_idx = FT_Get_Char_Index( face , ch.unicode());

      if ( kerning[col] && glyph_idx != 0 && previous != 0 )
      {
        FT_Vector  vec;

        FT_Get_Kerning( face, previous, glyph_idx, kerning_mode, &vec );

        column_x_start[col] += vec.x;
      }

      /* load glyph image into the slot (erase previous one) */
      error = FT_Load_Glyph( face, glyph_idx, load[col] );
      if ( error )
      {
        break;  /* ignore errors */
      }

      error = FT_Render_Glyph(face->glyph,
                                FT_RENDER_MODE_NORMAL);

      QImage glyphImage(face->glyph->bitmap.buffer,
                          face->glyph->bitmap.width,
                          face->glyph->bitmap.rows,
                          face->glyph->bitmap.pitch,
                          QImage::Format_Indexed8);

      QVector<QRgb> colorTable;
      for (int i = 0; i < 256; ++i)
      {
        colorTable << qRgba(0, 0, 0, i);
      }

      if (pixelMode_col[col] == FT_PIXEL_MODE_GRAY)
      {
        glyphImage.setColorTable(colorTable);
      } else
      {
        glyphImage.setColorTable(colorTable);
      }

      FT_Pos bottom = 0;

      if (count == 1)
      {
        FT_Pos bottom = face->glyph->metrics.height/64;
      }

      for (int n = 0; n < glyphImage.width(); n++)
      {
        for (int m = 0; m < glyphImage.height(); m++)
        {
          // be careful not to lose the alpha channel
          const QRgb p = glyphImage.pixel(n, m);
          const double r = qRed(p) / 255.0;
          const double g = qGreen(p) / 255.0;
          const double b = qBlue(p) / 255.0;
          const double a = qAlpha(p) / 255.0;
          painter->fillRect(QRectF(n + column_x_start[col]- 1 / lod / 2,
                                    m + height + bottom - face->glyph->metrics.horiBearingY/64- 1 / lod / 2,
                                    1/lod,
                                    1/lod),
                            QColor(
                                  255 * std::pow(r, 1/gamma),
                                  255 * std::pow(g, 1/gamma),
                                  255 * std::pow(b, 1/gamma),
                                  255 * std::pow(a, 1/gamma)));
        }
      }


      //painter->drawImage(column_x_start[col], height + bottom - face->glyph->metrics.horiBearingY/64,
      //                 glyphImage, 0, 0, -1, -1);


      column_x_start[col] += face->glyph->advance.x/64;

      if (column_x_start[col] >= column_x_end[col])
      { 
        height += (size->metrics.height + 4)/64;
        column_x_start[col] = column_x_temp[col];
      }

      previous = glyph_idx;
    }
  }
}


// end of glyphbitmap.cpp