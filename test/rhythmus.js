$(function() {
    var m = $("<div id='message'></div>");
    $('body').append(m);
    m.css({
        'position': 'absolute',
        'border-radius': '5px',
        'background': 'rgba(255, 255, 255, 0.5)',
        'color': '#000',
        'font-family': 'Consolas',
        'font-size': '12px',
        'padding': '5px',
        'line-height': '1.2',
        'width': '160px'
    });
    m.hide();

    $('.noteobject').on('mouseover', function(e)
    {
        var p = $(this).offset();
        var t = 'Type: Note';
        t += '<br>Time: ' + $(this).data('time');
        t += '<br>Beat: ' + $(this).data('beat');
        t += '<br>Value(Ch): ' + $(this).data('value');
        m.html(t);
        m.css({ 'left': e.pageX + 10, 'top': e.pageY + 10 });
        m.show();
    });
    $('.noteobject').on('mouseout', function() { m.hide(); });
})
