const choices = document.querySelectorAll('.compare-choice');
const image = document.querySelector('#comparison-image');
choices.forEach((choice) => choice.setAttribute('aria-pressed', choice.classList.contains('is-active')));
choices.forEach((choice) => choice.addEventListener('click', () => {
  choices.forEach((button) => {
    button.classList.remove('is-active');
    button.setAttribute('aria-pressed', 'false');
  });
  choice.classList.add('is-active');
  choice.setAttribute('aria-pressed', 'true');
  image.classList.add('is-changing');
  window.setTimeout(() => { image.src = choice.dataset.image; image.alt = choice.dataset.alt; image.classList.remove('is-changing'); }, 130);
}));
