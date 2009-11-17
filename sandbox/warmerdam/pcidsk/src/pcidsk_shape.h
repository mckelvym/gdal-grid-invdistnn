/******************************************************************************
 *
 * Purpose:  PCIDSK Vector Shape interface.  Declaration.
 * 
 ******************************************************************************
 * Copyright (c) 2009
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef __INCLUDE_PCIDSK_SHAPE_H
#define __INCLUDE_PCIDSK_SHAPE_H

#include <string>
#include <vector>

namespace PCIDSK
{

    const static int NullShapeId = -1;
    typedef int32 ShapeId;

    typedef struct 
    {
        double x;
        double y;
        double z;
    } ShapeVertex;

    typedef enum  // These deliberately match GDBFieldType values.
    {
        FieldTypeNone = 0,
        FieldTypeFloat = 1,
        FieldTypeDouble = 2,
        FieldTypeString = 3,
        FieldTypeInteger = 4,
        FieldTypeCountedInt = 5
    } ShapeFieldType;

    inline std::string ShapeFieldTypeName( ShapeFieldType type )
    {
        switch( type ) {
          case FieldTypeNone: return "None";
          case FieldTypeFloat: return "Float";
          case FieldTypeDouble: return "Double";
          case FieldTypeString: return "String";
          case FieldTypeInteger: return "Integer";
          case FieldTypeCountedInt: return "CountedInt";
        }
        return "";
    }

    class ShapeField
    {
      private:
        ShapeFieldType  type; // use FieldTypeNone for NULL fields.

        union
        {
            float	float_val;
            double	double_val;
            char       *string_val;
            int32       integer_val;
            int32      *integer_list_val;
        } v;
        
      public:
        ShapeField() 
            { v.string_val = NULL; type = FieldTypeNone; }

        ShapeField( const ShapeField &src ) { *this = src; }

        ~ShapeField() 
            { Clear(); }

        ShapeField &operator=( const ShapeField &src )
            {
                switch( src.GetType() )
                {
                  case FieldTypeFloat: 
                    SetValue( src.GetValueFloat() );
                    break;
                  case FieldTypeDouble: 
                    SetValue( src.GetValueDouble() );
                    break;
                  case FieldTypeInteger: 
                    SetValue( src.GetValueInteger() );
                    break;
                  case FieldTypeCountedInt: 
                    SetValue( src.GetValueCountedInt() );
                    break;
                  case FieldTypeString:
                    SetValue( src.GetValueString() );
                    break;
                  case FieldTypeNone:
                    Clear();
                    break;
                }
                return *this;
            }

        void Clear()
            { 
                if( (type == FieldTypeString || type == FieldTypeCountedInt)
                    && v.string_val != NULL ) 
                {
                    free( v.string_val );
                    v.string_val = NULL;
                }
                type = FieldTypeNone;
            }

        ShapeFieldType  GetType() const
            { return type; }

        void SetValue( int32 val ) 
            { 
                Clear(); 
                type = FieldTypeInteger; 
                v.integer_val = val; 
            }

        void SetValue( const std::vector<int32> &val )
            { 
                Clear();
                type = FieldTypeCountedInt; 
                v.integer_list_val = (int32*)
                    malloc(sizeof(int32) * (val.size()+1) );
                v.integer_list_val[0] = val.size();
                memcpy( v.integer_list_val+1, &(val[0]), 
                        sizeof(int32) * val.size() ); 
            }

        void SetValue( const std::string &val )
            { 
                Clear(); 
                type = FieldTypeString; 
                v.string_val = strdup(val.c_str()); 
            }

        void SetValue( double val )
            { 
                Clear(); 
                type = FieldTypeDouble; 
                v.double_val = val; 
            }

        void SetValue( float val )
            { 
                Clear(); 
                type = FieldTypeFloat; 
                v.float_val = val; 
            }

        int32 GetValueInteger() const
            { if( type == FieldTypeInteger ) return v.integer_val; else return 0; }
        std::vector<int32> GetValueCountedInt() const
            { 
                std::vector<int32> result;
                if( type == FieldTypeCountedInt )
                {
                    result.resize( v.integer_list_val[0] );
                    memcpy( &(result[0]), &(v.integer_list_val[1]), 
                            (v.integer_list_val[0]) * sizeof(int32) );
                }
                return result;
            }
        std::string GetValueString() const
            { if( type == FieldTypeString ) return v.string_val; else return ""; }
        float GetValueFloat() const
            { if( type == FieldTypeFloat ) return v.float_val; else return 0.0; }
        double GetValueDouble() const
            { if( type == FieldTypeDouble ) return v.double_val; else return 0.0; }
    };

}; // end namespace PCIDSK

#endif // __INCLUDE_PCIDSK_SHAPE_H
