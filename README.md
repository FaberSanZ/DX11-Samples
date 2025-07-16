##               DirectX 11 Samples
  
</h1>


  ##              



## Overview
This is designed for the DirectX 11 learning process, I mainly use it to experiment with graphical or computing techniques and should not be used as a cerium product as it may have memory leaks and faulty or poorly optimized implementations.




## Examples


Example | Details
---------|--------
<img src="Screenshots/ClearScreen.png" width=380> | [Clear Screen](Src/ClearScreen)<br> This example shows how to configure the device and clear the color.
<img src="Screenshots/Pipeline.png" width=380> | [Pipeline](Src/Pipeline)<br> We will start drawing geometry onto the screen in this tutorial. We will learn more about Pipeline
<img src="Screenshots/VertexBuffer.png" width=380> | [VertexBuffer](Src/VertexBuffer)<br> Let's get some color in our scene. In this tutorial we will add color to our vertices to color our triangle. This involves updating the vertex shader to pass the color to the pixel shader, the pixel shader to output the color passed to it, the vertex structure to add a color attribute, and the input layout to include a color input element.
<img src="Screenshots/IndexBuffer.png" width=380> | [IndexBuffer](Src/IndexBuffer)<br> In this tutorial we will learn how to use indices to define our triangles. This is useful because we can remove duplicate vertices, as many times the same vertex is used in multiple triangles.
<img src="Screenshots/DepthTests.png" width=380> | [DepthTests](Src/DepthTests)<br> We will create a depth/stencil buffer, then create a depth/stencil view which we bind to the OM stage of the pipeline
