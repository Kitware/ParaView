// Hide the paragraph from index.html telling to enable `PARAVIEW_USE_WEBENGINE`
// if the JS script is actually executed
document.getElementById("no-js").innerHTML = "";

// JS for the carousel
var slideIndex = 1;
showSlides(slideIndex);

function plusSlides(n)
{
  showSlides(slideIndex += n);
}

function showSlides(n)
{
  var i;
  var slides = document.getElementsByClassName("mySlides");
  if (n > slides.length)
  {
    slideIndex = 1;
  }
  if (n < 1)
  {
    slideIndex = slides.length;
  }

  for (i = 0; i < slides.length; i++)
  {
      slides[i].style.display = "none";
  }
  slides[slideIndex-1].style.display = "block";
}
