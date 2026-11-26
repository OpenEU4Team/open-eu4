#pragma once

template <typename T>
class CPoint;

class CPdxWindow
{
public:
    inline CPdxWindow() = default;                                                             
    virtual ~CPdxWindow();                                                                            
    virtual void SetWindowHandle(void *pHandle);                                                                      
    virtual void *GetWindowHandle();                                                                                 
    virtual bool HasWindowFocus();                                                                                  
    virtual void SetWindowFocus(bool bFocus);                                                                       
    virtual bool SetWindowIcon(void *pIcon, int nSize);                                                              
    virtual CPoint<int> GetSize() const;                                                                     
    static void SetInstance(CPdxWindow *pWindow);                                                       
};
